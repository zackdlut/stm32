#include "drivers/systick.h"

#include "stm32f103.h"

static volatile uint32_t s_ticks;
static void (*volatile s_callback)(void);

void systick_init(uint32_t core_hz)
{
    SYSTICK->LOAD = core_hz / 1000u - 1u;
    SYSTICK->VAL = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

void SysTick_Handler(void)
{
    s_ticks++;
    if (s_callback != 0) {
        s_callback();
    }
}

uint32_t millis(void)
{
    return s_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    /* Unsigned subtraction stays correct when s_ticks wraps after ~49 days. */
    while ((uint32_t)(s_ticks - start) < ms) {
        wait_for_interrupt();
    }
}

void systick_set_callback(void (*callback)(void))
{
    s_callback = callback;
}
