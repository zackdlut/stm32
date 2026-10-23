#include <stdint.h>

#define RCC_BASE    0x40023800u
#define GPIOA_BASE  0x40020000u
#define GPIOD_BASE  0x40020C00u
#define USART2_BASE 0x40004400u

#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30u))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40u))

#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00u))
#define GPIOA_AFRL  (*(volatile uint32_t *)(GPIOA_BASE + 0x20u))

#define GPIOD_MODER (*(volatile uint32_t *)(GPIOD_BASE + 0x00u))
#define GPIOD_ODR   (*(volatile uint32_t *)(GPIOD_BASE + 0x14u))

#define USART2_SR  (*(volatile uint32_t *)(USART2_BASE + 0x00u))
#define USART2_DR  (*(volatile uint32_t *)(USART2_BASE + 0x04u))
#define USART2_BRR (*(volatile uint32_t *)(USART2_BASE + 0x08u))
#define USART2_CR1 (*(volatile uint32_t *)(USART2_BASE + 0x0Cu))

#define USART_SR_TXE (1u << 7)
#define LED_PIN      (1u << 12)

static void uart_putc(char c)
{
    while ((USART2_SR & USART_SR_TXE) == 0) {
    }
    USART2_DR = (uint8_t)c;
}

static void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

static void delay(volatile uint32_t cycles)
{
    while (cycles-- != 0) {
        __asm__ volatile("nop");
    }
}

/* STM32F4 Discovery: green LED is PD12, USART2 on PA2/PA3. */
int main(void)
{
    uint32_t count = 0;

    RCC_AHB1ENR |= (1u << 0) | (1u << 3);
    RCC_APB1ENR |= (1u << 17);

    GPIOA_MODER = (GPIOA_MODER & ~((3u << 4) | (3u << 6))) | (2u << 4) | (2u << 6);
    GPIOA_AFRL = (GPIOA_AFRL & ~((0xFu << 8) | (0xFu << 12))) | (7u << 8) | (7u << 12);

    GPIOD_MODER = (GPIOD_MODER & ~(3u << 24)) | (1u << 24);

    /* HSI 16 MHz, 115200 baud: mantissa 138, fraction 14. */
    USART2_BRR = 0x08AEu;
    USART2_CR1 = (1u << 13) | (1u << 3) | (1u << 2);

    for (;;) {
        GPIOD_ODR ^= LED_PIN;
        uart_puts("Hello from STM32F407 on Renode, count=0x");
        uart_putc("0123456789ABCDEF"[(count >> 4) & 0xFu]);
        uart_putc("0123456789ABCDEF"[count & 0xFu]);
        uart_puts("\r\n");
        count++;
        delay(200000u);
    }
}
