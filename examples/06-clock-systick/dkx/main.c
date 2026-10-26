/*
 * Lesson 6, Dikuoxing STM32F103C8T6 version: clock tree and SysTick.
 *
 * Phase 1 runs from the 8 MHz HSI with a 1 kHz SysTick interrupt.
 * Phase 2 switches to 72 MHz (8 MHz HSE crystal -> PLL x9), reprograms the
 * bus prescalers, flash wait states, SysTick and the UART baud divider.
 * The same busy loop is timed in both phases to show the speed-up.
 *
 * Unlike the F4, the F103 PLL has no separate configuration register: source,
 * HSE predivider and multiplier are fields of RCC_CFGR.
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/uart.h"

#define HSE_HZ 8000000u

static uint32_t g_core_hz = HSI_HZ;
static volatile uint32_t g_ticks;

/* ---- SysTick: 24-bit down-counter in the Cortex-M core ------------------- */
static void systick_start_1khz(uint32_t core_hz)
{
    SYSTICK->LOAD = core_hz / 1000u - 1u;   /* counts LOAD..0, so LOAD+1 cycles per tick */
    SYSTICK->VAL  = 0;                      /* any write clears the counter */
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE  /* processor clock, not HCLK/8 */
                  | SYSTICK_CTRL_TICKINT    /* raise the SysTick exception at 0 */
                  | SYSTICK_CTRL_ENABLE;
}

void SysTick_Handler(void)
{
    g_ticks++;
}

static uint32_t millis(void)
{
    return g_ticks;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = g_ticks;
    while ((uint32_t)(g_ticks - start) < ms) {
        wait_for_interrupt();               /* sleep until the next interrupt */
    }
}

/* ---- Reading the clock tree back from RCC -------------------------------- */
static uint32_t sysclk_from_registers(void)
{
    uint32_t cfgr = RCC->CFGR;
    switch ((cfgr & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos) {
    case 0:
        return HSI_HZ;
    case 1:
        return HSE_HZ;
    default: {
        uint32_t src;
        if (cfgr & RCC_CFGR_PLLSRC) {
            src = (cfgr & RCC_CFGR_PLLXTPRE) ? HSE_HZ / 2u : HSE_HZ;
        } else {
            src = HSI_HZ / 2u;              /* HSI always enters the PLL halved */
        }
        uint32_t mul = ((cfgr >> RCC_CFGR_PLLMUL_Pos) & 0xFu) + 2u;
        return src * (mul > 16u ? 16u : mul);
    }
    }
}

/* ---- 72 MHz from the 8 MHz crystal ---------------------------------------
 *
 *   HSE 8 MHz --PLLXTPRE /1--> 8 MHz --PLLMUL x9--> SYSCLK 72 MHz
 *   AHB  /1 -> HCLK  72 MHz (core, SysTick)
 *   APB1 /2 -> PCLK1 36 MHz (its maximum; TIM2-4 run at 2 x 36 = 72 MHz)
 *   APB2 /1 -> PCLK2 72 MHz (GPIO, USART1, ADC, TIM1)
 */
static void clock_init_72mhz(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0) {
    }

    /* FLASH cannot answer in one 72 MHz cycle: 2 wait states above 48 MHz,
     * and the prefetch buffer hides most of them. */
    FLASH->ACR = FLASH_ACR_PRFTBE | 2u;

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~((0xFu << RCC_CFGR_HPRE_Pos) | (7u << RCC_CFGR_PPRE1_Pos) | (7u << RCC_CFGR_PPRE2_Pos) |
              RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | (0xFu << RCC_CFGR_PLLMUL_Pos));
    cfgr |= (0u << RCC_CFGR_HPRE_Pos)       /* AHB  /1 */
          | (4u << RCC_CFGR_PPRE1_Pos)      /* APB1 /2 */
          | (0u << RCC_CFGR_PPRE2_Pos)      /* APB2 /1 */
          | RCC_CFGR_PLLSRC                 /* PLL input: HSE, not divided */
          | ((9u - 2u) << RCC_CFGR_PLLMUL_Pos);   /* x9 */
    RCC->CFGR = cfgr;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }

    g_core_hz = 72000000u;
}

static void busy_loop(void)
{
    for (volatile uint32_t i = 0; i < 100000u; i++) {
    }
}

static void phase(const char *name)
{
    printf("\n[%s] SYSCLK = %lu Hz (from RCC registers)\n", name, (unsigned long)sysclk_from_registers());

    uint32_t t0 = millis();
    busy_loop();
    printf("[%s] busy loop of 100000 iterations took %lu ms\n", name, (unsigned long)(millis() - t0));

    for (int i = 0; i < 4; i++) {
        led_toggle();
        printf("[%s] t=%4lu ms  PC13 LED %s\n", name, (unsigned long)millis(), led_is_on() ? "on" : "off");
        delay_ms(250);
    }
}

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    systick_start_1khz(g_core_hz);

    phase("HSI 8 MHz");

    clock_init_72mhz();
    /* Everything derived from a clock must be recomputed after the switch.
     * USART1 is on APB2, which now runs at the full 72 MHz. */
    systick_start_1khz(g_core_hz);
    uart_init(g_core_hz, 115200);

    phase("PLL 72 MHz");

    printf("\ndone at t=%lu ms\n", (unsigned long)millis());
    for (;;) {
        wait_for_interrupt();
    }
}
