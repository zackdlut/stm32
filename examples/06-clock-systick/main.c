/*
 * Lesson 6: clock tree and SysTick.
 *
 * Phase 1 runs from the 16 MHz HSI with a 1 kHz SysTick interrupt.
 * Phase 2 switches to 168 MHz (8 MHz HSE crystal -> PLL), reprograms the
 * bus prescalers, flash wait states, SysTick and the UART baud divider.
 * The same busy loop is timed in both phases to show the speed-up.
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
    switch ((RCC->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos) {
    case 0:
        return HSI_HZ;
    case 1:
        return HSE_HZ;
    default: {
        uint32_t pll = RCC->PLLCFGR;
        uint32_t src = (pll & RCC_PLLCFGR_SRC_HSE) ? HSE_HZ : HSI_HZ;
        uint32_t m = pll & 0x3Fu;
        uint32_t n = (pll >> 6) & 0x1FFu;
        uint32_t p = (((pll >> 16) & 3u) + 1u) * 2u;
        return src / m * n / p;
    }
    }
}

/* ---- 168 MHz from the 8 MHz crystal --------------------------------------
 *
 *   HSE 8 MHz --/M=8--> 1 MHz --xN=336--> VCO 336 MHz --/P=2--> SYSCLK 168 MHz
 *                                                    \--/Q=7--> 48 MHz (USB)
 *   AHB  /1 -> HCLK  168 MHz (core, SysTick, GPIO)
 *   APB1 /4 -> PCLK1  42 MHz (USART2, TIM2-7 run at 2 x 42 = 84 MHz)
 *   APB2 /2 -> PCLK2  84 MHz
 */
static void clock_init_168mhz(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0) {
    }

    /* FLASH cannot answer in one 168 MHz cycle: 5 wait states at 2.7-3.6 V. */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | 5u;

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~((0xFu << RCC_CFGR_HPRE_Pos) | (7u << RCC_CFGR_PPRE1_Pos) | (7u << RCC_CFGR_PPRE2_Pos));
    cfgr |= (0u << RCC_CFGR_HPRE_Pos)       /* AHB  /1 */
          | (5u << RCC_CFGR_PPRE1_Pos)      /* APB1 /4 */
          | (4u << RCC_CFGR_PPRE2_Pos);     /* APB2 /2 */
    RCC->CFGR = cfgr;

    /* Read-modify-write: PLLCFGR has reserved bits that must keep their value. */
    const uint32_t fields = 0x3Fu | (0x1FFu << 6) | (3u << 16) | RCC_PLLCFGR_SRC_HSE | (0xFu << 24);
    RCC->PLLCFGR = (RCC->PLLCFGR & ~fields)
                 | 8u                       /* M */
                 | (336u << 6)              /* N */
                 | (0u << 16)               /* P = 2 */
                 | RCC_PLLCFGR_SRC_HSE
                 | (7u << 24);              /* Q */

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }

    g_core_hz = 168000000u;
}

static void busy_loop(void)
{
    for (volatile uint32_t i = 0; i < 200000u; i++) {
    }
}

static void phase(const char *name)
{
    printf("\n[%s] SYSCLK = %lu Hz (from RCC registers)\n", name, (unsigned long)sysclk_from_registers());

    uint32_t t0 = millis();
    busy_loop();
    printf("[%s] busy loop of 200000 iterations took %lu ms\n", name, (unsigned long)(millis() - t0));

    for (int i = 0; i < 4; i++) {
        led_toggle(LED_GREEN);
        printf("[%s] t=%4lu ms  green LED %s\n", name, (unsigned long)millis(),
               (LED_PORT->ODR & BIT(LED_GREEN)) ? "on" : "off");
        delay_ms(250);
    }
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    systick_start_1khz(g_core_hz);

    phase("HSI 16 MHz");

    clock_init_168mhz();
    /* Everything derived from a clock must be recomputed after the switch. */
    systick_start_1khz(g_core_hz);
    uart_init(g_core_hz / 4u, 115200);

    phase("PLL 168 MHz");

    printf("\ndone at t=%lu ms\n", (unsigned long)millis());
    for (;;) {
        wait_for_interrupt();
    }
}
