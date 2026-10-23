/*
 * Lesson 8: USART from scratch, printf and reading input.
 *
 * Everything drivers/uart.c hides is written out here: pin multiplexing,
 * baud rate divider, the TXE/RXNE/TC flags, and the hook that lets newlib's
 * printf reach the hardware.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "stm32f407.h"

#define PCLK1_HZ 16000000u
#define BAUD     115200u

/* ---- Driver -------------------------------------------------------------- */
static void usart2_init(uint32_t pclk_hz, uint32_t baud)
{
    /* 1. Clocks: GPIOA on AHB1, USART2 on APB1. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* 2. PA2 = TX, PA3 = RX: alternate function mode, AF7 (USART1..3). */
    GPIOA->MODER  = (GPIOA->MODER & ~((3u << 4) | (3u << 6))) | (2u << 4) | (2u << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((0xFu << 8) | (0xFu << 12))) | (7u << 8) | (7u << 12);
    GPIOA->PUPDR  = (GPIOA->PUPDR & ~(3u << 6)) | (1u << 6);   /* idle-high RX: pull-up */

    /* 3. Baud rate: USARTDIV = pclk / (16 * baud), stored as 12.4 fixed point,
     *    which is the same as round(pclk / baud). */
    USART2->BRR = (pclk_hz + baud / 2u) / baud;

    /* 4. Frame format 8N1 is the reset default (CR1.M=0, CR1.PCE=0, CR2.STOP=00).
     *    Enable transmitter, receiver, then the USART itself. */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void usart2_write_byte(uint8_t byte)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {   /* wait: data register empty */
    }
    USART2->DR = byte;                           /* hardware moves it to the shift register */
}

static uint8_t usart2_read_byte(void)
{
    while ((USART2->SR & USART_SR_RXNE) == 0) {  /* wait: a byte has arrived */
    }
    return (uint8_t)USART2->DR;                  /* reading DR clears RXNE */
}

static void usart2_flush(void)
{
    while ((USART2->SR & USART_SR_TC) == 0) {    /* wait: last stop bit has left the pin */
    }
}

/* newlib: printf -> vfprintf -> _write (common/syscalls.c) -> __io_putchar */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        usart2_write_byte('\r');
    }
    usart2_write_byte((uint8_t)ch);
    return ch;
}

/* ---- Demo ---------------------------------------------------------------- */
static void print_baud_table(void)
{
    static const uint32_t bauds[] = {9600, 115200, 921600};

    printf("\nBRR for PCLK1 = %lu Hz (oversampling 16)\n", (unsigned long)PCLK1_HZ);
    printf("  baud     BRR     mantissa.fraction  actual baud  error\n");
    for (unsigned i = 0; i < sizeof bauds / sizeof bauds[0]; i++) {
        uint32_t brr = (PCLK1_HZ + bauds[i] / 2u) / bauds[i];
        uint32_t actual = PCLK1_HZ / brr;
        int32_t error_ppm = (int32_t)(((int64_t)actual - bauds[i]) * 1000000 / bauds[i]);
        uint32_t magnitude = (uint32_t)(error_ppm < 0 ? -error_ppm : error_ppm);
        printf("  %-7lu  0x%04lx  %4lu.%-2lu             %-11lu  %c%lu.%02lu%%\n",
               (unsigned long)bauds[i], (unsigned long)brr,
               (unsigned long)(brr >> 4), (unsigned long)(brr & 0xFu),
               (unsigned long)actual, error_ppm < 0 ? '-' : '+',
               (unsigned long)(magnitude / 10000), (unsigned long)(magnitude / 100 % 100));
    }
}

static void print_formats(void)
{
    int temperature_centi = -1234;   /* -12.34 C as fixed point */
    printf("\nprintf formats (newlib-nano, no float by default):\n");
    printf("  %%d  %d   %%u  %u   %%x  0x%08x   %%s  %s   %%c  %c\n", -42, 42u, 0xBEEFu, "text", 'A');
    printf("  fixed point: %d.%02d C\n", temperature_centi / 100,
           (temperature_centi < 0 ? -temperature_centi : temperature_centi) % 100);
}

static size_t read_line(char *buf, size_t size)
{
    size_t n = 0;
    for (;;) {
        char c = (char)usart2_read_byte();
        if (c == '\r' || c == '\n') {
            break;
        }
        if (n + 1 < size) {
            buf[n++] = c;
            usart2_write_byte((uint8_t)c);       /* echo, like a terminal */
        }
    }
    buf[n] = '\0';
    printf("\n");
    return n;
}

int main(void)
{
    usart2_init(PCLK1_HZ, BAUD);

    /* newlib buffers stdout until '\n'. A prompt without a newline would sit
     * in the buffer while we echo input directly, so turn buffering off. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("USART2 up: BRR=0x%04lx CR1=0x%04lx\n", (unsigned long)USART2->BRR, (unsigned long)USART2->CR1);
    print_baud_table();
    print_formats();

    char line[64];
    for (int i = 0; i < 2; i++) {
        printf("\ntype a line> ");
        size_t len = read_line(line, sizeof line);
        for (size_t k = 0; k < len; k++) {
            line[k] = (char)toupper((unsigned char)line[k]);
        }
        printf("got %u chars, upper case: %s\n", (unsigned)len, line);
    }

    printf("\nbye\n");
    usart2_flush();
    for (;;) {
    }
}
