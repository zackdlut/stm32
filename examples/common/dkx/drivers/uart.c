#include "drivers/uart.h"

#include "drivers/gpio.h"
#include "stm32f103.h"

#define UART_TX_PIN 9u
#define UART_RX_PIN 10u

void uart_init(uint32_t pclk_hz, uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* USART1_TX is PA9 without any remap: the pin only has to be switched to
     * "alternate function output". RX is an ordinary input. */
    gpio_config(GPIOA, UART_TX_PIN, GPIO_CFG_AF_PP_50MHZ);
    gpio_set_pull(GPIOA, UART_RX_PIN, GPIO_PULL_UP);

    /* With 16x oversampling BRR holds USARTDIV in 12.4 fixed point, which is
     * simply pclk / baud rounded to the nearest integer. */
    USART1->BRR = (pclk_hz + baud / 2u) / baud;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0) {
    }
    USART1->DR = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s++);
    }
}

int uart_getc_nonblock(void)
{
    if ((USART1->SR & USART_SR_RXNE) == 0) {
        return -1;
    }
    return (int)(USART1->DR & 0xFFu);
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
