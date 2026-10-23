/* USART baud rate arithmetic (oversampling by 16), as in chapter 8. */
#ifndef BAUD_H
#define BAUD_H

#include <stdint.h>

uint32_t uart_brr(uint32_t pclk_hz, uint32_t baud);             /* rounded to nearest */
uint32_t uart_baud_from_brr(uint32_t pclk_hz, uint32_t brr);    /* what the line really runs at */
int32_t uart_baud_error_ppm(uint32_t pclk_hz, uint32_t baud);   /* (actual - wanted) / wanted */

#endif
