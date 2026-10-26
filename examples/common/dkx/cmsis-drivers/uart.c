#include "uart.h"

#include "board.h"

void uart_init(uint32_t pclk_hz, uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    gpio_config(GPIOA, 9, GPIO_CFG_AF_PP_50MHZ);   /* PA9 = USART1_TX */
    gpio_set_pull_up(GPIOA, 10);                    /* PA10 = USART1_RX, idle high */

    USART1->BRR = (pclk_hz + baud / 2u) / baud;     /* pclk = PCLK2 */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0) {
    }
    USART1->DR = (uint8_t)c;
}

int uart_getc_nonblock(void)
{
    if ((USART1->SR & USART_SR_RXNE) == 0) {
        return -1;
    }
    return (int)(USART1->DR & 0xFFu);
}

void uart_flush(void)
{
    while ((USART1->SR & USART_SR_TC) == 0) {
    }
}

int __io_putchar(int ch)
{
    if (ch == '\n') {
        uart_putc('\r');
    }
    uart_putc((char)ch);
    return ch;
}
