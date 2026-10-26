/*
 * 1 kHz SysTick time base: millis() and delay_ms().
 */
#ifndef CMSIS_DRIVERS_TICK_H
#define CMSIS_DRIVERS_TICK_H

#include <stdint.h>

void tick_init(uint32_t core_hz);
uint32_t millis(void);
void delay_ms(uint32_t ms);

#endif
