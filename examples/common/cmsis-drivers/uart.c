#include "uart.h"

#include "board.h"

void uart_init(uint32_t pclk_hz, uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_set_af(GPIOA, 2, 7);   /* PA2 = USART2_TX, AF7 */
    gpio_set_af(GPIOA, 3, 7);   /* PA3 = USART2_RX, AF7 */
    gpio_set_pull(GPIOA, 3, GPIO_PULL_UP);

    USART2->BRR = (pclk_hz + baud / 2u) / baud;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
    USART2->DR = (uint8_t)c;
}

int uart_getc_nonblock(void)
{
    if ((USART2->SR & USART_SR_RXNE) == 0) {
        return -1;
    }
    return (int)(USART2->DR & 0xFFu);
}

void uart_flush(void)
{
    while ((USART2->SR & USART_SR_TC) == 0) {
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
