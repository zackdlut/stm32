/*
 * 1 kHz system tick built on the Cortex-M SysTick timer (lesson 6).
 */
#ifndef DRIVERS_SYSTICK_H
#define DRIVERS_SYSTICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void systick_init(uint32_t core_hz);
uint32_t millis(void);
void delay_ms(uint32_t ms);

/* Optional function called from the SysTick interrupt every millisecond. */
void systick_set_callback(void (*callback)(void));

#ifdef __cplusplus
}
#endif

#endif
