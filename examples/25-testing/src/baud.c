#include "baud.h"

uint32_t uart_brr(uint32_t pclk_hz, uint32_t baud)
{
    return (pclk_hz + baud / 2u) / baud;
}

uint32_t uart_baud_from_brr(uint32_t pclk_hz, uint32_t brr)
{
    return pclk_hz / brr;
}

int32_t uart_baud_error_ppm(uint32_t pclk_hz, uint32_t baud)
{
    int64_t actual = uart_baud_from_brr(pclk_hz, uart_brr(pclk_hz, baud));
    return (int32_t)((actual - (int64_t)baud) * 1000000 / (int64_t)baud);
}
