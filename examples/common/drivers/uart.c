#include "drivers/uart.h"

#include "drivers/gpio.h"
#include "stm32f407.h"

#define UART_TX_PIN 2u
#define UART_RX_PIN 3u
#define GPIO_AF7_USART2 7u

void uart_init(uint32_t pclk_hz, uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_set_af(GPIOA, UART_TX_PIN, GPIO_AF7_USART2);
    gpio_set_af(GPIOA, UART_RX_PIN, GPIO_AF7_USART2);
    gpio_set_pull(GPIOA, UART_RX_PIN, GPIO_PULL_UP);

    /* With 16x oversampling BRR holds USARTDIV in 12.4 fixed point, which is
     * simply pclk / baud rounded to the nearest integer. */
    USART2->BRR = (pclk_hz + baud / 2u) / baud;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
    USART2->DR = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

int uart_getc_nonblock(void)
{
    if ((USART2->SR & USART_SR_RXNE) == 0) {
        return -1;
    }
    return (int)(USART2->DR & 0xFFu);
}

/* printf -> _write (syscalls.c) -> __io_putchar. Terminals expect CR LF. */
int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart_putc('\r');
    }
    uart_putc((char)ch);
    return ch;
}
