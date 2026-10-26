/*
 * USART1 on PA9/PA10, polled. printf() reaches it through __io_putchar.
 */
#ifndef CMSIS_DRIVERS_UART_H
#define CMSIS_DRIVERS_UART_H

#include <stdint.h>

void uart_init(uint32_t pclk_hz, uint32_t baud);
void uart_putc(char c);
int uart_getc_nonblock(void);   /* -1 when nothing has arrived */
void uart_flush(void);          /* wait until the last stop bit has left */

#endif
