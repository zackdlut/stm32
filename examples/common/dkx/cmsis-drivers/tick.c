#include "tick.h"

#include "stm32f1xx.h"

static volatile uint32_t s_ticks;

void tick_init(uint32_t core_hz)
{
    SysTick_Config(core_hz / 1000u);
}

/* Weak so that lessons with their own tick source (FreeRTOS) can replace it. */
__attribute__((weak)) void SysTick_Handler(void)
{
    s_ticks++;
}

uint32_t millis(void)
{
    return s_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((uint32_t)(s_ticks - start) < ms) {
        __WFI();
    }
}
