/*
 * Polling driver for the debug serial port (USART2 on PA2/PA3, 8N1).
 * Lesson 8 builds the same driver step by step.
 */
#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* pclk_hz: clock of the APB1 bus USART2 hangs on (16 MHz after reset). */
void uart_init(uint32_t pclk_hz, uint32_t baud);
void uart_putc(char c);
void uart_puts(const char *s);

/* Returns the received byte, or -1 if nothing has arrived. */
int uart_getc_nonblock(void);

#ifdef __cplusplus
}
#endif

#endif
