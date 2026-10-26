/*
 * Lesson 8, Dikuoxing STM32F103C8T6 version: USART from scratch, printf and
 * reading input.
 *
 * Everything drivers/uart.c hides is written out here: pin configuration,
 * baud rate divider, the TXE/RXNE/TC flags, and the hook that lets newlib's
 * printf reach the hardware.
 *
 * The board's serial header is USART1 on PA9/PA10. USART1 sits on APB2, so its
 * baud divider is computed from PCLK2 (8 MHz after reset, 72 MHz at full speed).
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "stm32f103.h"

#define PCLK2_HZ 8000000u
#define BAUD     115200u

/* ---- Driver -------------------------------------------------------------- */
static void usart1_init(uint32_t pclk_hz, uint32_t baud)
{
    /* 1. Clocks: GPIOA and USART1 are both on APB2. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* 2. Pins. The F1 has no per-pin AF number: a pin set to "alternate
     *    function output" is driven by whichever peripheral owns it by default
     *    (USART1_TX = PA9). Inputs need no AF setting at all, USART1 RX simply
     *    reads PA10.
     *      PA9  CRH[7:4]   = 0xB  AF push-pull, 50 MHz
     *      PA10 CRH[11:8]  = 0x8  input with pull, ODR10 = 1 selects pull-up */
    GPIOA->CRH = (GPIOA->CRH & ~((0xFu << 4) | (0xFu << 8))) | (0xBu << 4) | (0x8u << 8);
    GPIOA->BSRR = BIT(10);

    /* 3. Baud rate: USARTDIV = pclk / (16 * baud), stored as 12.4 fixed point,
     *    which is the same as round(pclk / baud). */
    USART1->BRR = (pclk_hz + baud / 2u) / baud;

    /* 4. Frame format 8N1 is the reset default (CR1.M=0, CR1.PCE=0, CR2.STOP=00).
     *    Enable transmitter, receiver, then the USART itself. */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void usart1_write_byte(uint8_t byte)
{
    while ((USART1->SR & USART_SR_TXE) == 0) {   /* wait: data register empty */
    }
    USART1->DR = byte;                           /* hardware moves it to the shift register */
}

static uint8_t usart1_read_byte(void)
{
    while ((USART1->SR & USART_SR_RXNE) == 0) {  /* wait: a byte has arrived */
    }
    return (uint8_t)USART1->DR;                  /* reading DR clears RXNE */
}

static void usart1_flush(void)
{
    while ((USART1->SR & USART_SR_TC) == 0) {    /* wait: last stop bit has left the pin */
    }
}

/* newlib: printf -> vfprintf -> _write (common/syscalls.c) -> __io_putchar */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        usart1_write_byte('\r');
    }
    usart1_write_byte((uint8_t)ch);
    return ch;
}

/* ---- Demo ---------------------------------------------------------------- */
static void print_baud_table(uint32_t pclk_hz)
{
    static const uint32_t bauds[] = {9600, 115200, 921600};

    printf("\nBRR for PCLK2 = %lu Hz (oversampling 16)\n", (unsigned long)pclk_hz);
    printf("  baud     BRR     mantissa.fraction  actual baud  error\n");
    for (unsigned i = 0; i < sizeof bauds / sizeof bauds[0]; i++) {
        uint32_t brr = (pclk_hz + bauds[i] / 2u) / bauds[i];
        uint32_t actual = pclk_hz / brr;
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
        char c = (char)usart1_read_byte();
        if (c == '\r' || c == '\n') {
            break;
        }
        if (n + 1 < size) {
            buf[n++] = c;
            usart1_write_byte((uint8_t)c);       /* echo, like a terminal */
        }
    }
    buf[n] = '\0';
    printf("\n");
    return n;
}

int main(void)
{
    usart1_init(PCLK2_HZ, BAUD);

    /* newlib buffers stdout until '\n'. A prompt without a newline would sit
     * in the buffer while we echo input directly, so turn buffering off. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("USART1 up: BRR=0x%04lx CR1=0x%04lx\n", (unsigned long)USART1->BRR, (unsigned long)USART1->CR1);
    print_baud_table(PCLK2_HZ);
    print_baud_table(72000000u);    /* after lesson 6's switch to 72 MHz */
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
    usart1_flush();
    for (;;) {
    }
}
