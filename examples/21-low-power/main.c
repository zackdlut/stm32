/*
 * Lesson 21: low-power modes.
 *
 * The same job (wake up every 100 ms, blink, go back to waiting) is done ten
 * times in three ways:
 *
 *   phase 1  busy-wait: the CPU spins on millis()          -> Run mode all the time
 *   phase 2  WFI between SysTick interrupts                -> Sleep mode (core clock off)
 *   phase 3  Stop mode, woken by the RTC wakeup timer      -> all 1.2 V domain clocks off
 *
 * The phase number is written to RTC backup register 0. stimulus.resc hooks
 * that write and prints how many instructions the CPU executed per phase,
 * which is a stand-in for an ammeter in the simulator.
 */
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define JOBS 10

static volatile uint32_t g_rtc_wakeups;

static void mark_phase(uint32_t phase)
{
    uart_flush();                               /* finish printing before measuring */
    RTC->BKP0R = phase;
}

static void job(void)
{
    led_toggle(LED_GREEN);
}

/* ---- RTC wakeup timer every 100 ms (LSE 32768 Hz / 16 = 2048 Hz) ---------- */
static void rtc_wakeup_init_100ms(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;
    RCC->BDCR |= RCC_BDCR_LSEON;
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {
    }
    RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    RTC->CR &= ~RTC_CR_WUTE;
    while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {
    }
    RTC->CR &= ~RTC_CR_WUCKSEL;                 /* 000: RTCCLK / 16 = 2048 Hz */
    RTC->WUTR = 2048u / 10u - 1u;               /* 205 ticks = 100.1 ms */

    EXTI->IMR |= EXTI_IMR_MR22;
    EXTI->RTSR |= EXTI_RTSR_TR22;
    NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

void RTC_WKUP_IRQHandler(void)
{
    RTC->ISR &= ~RTC_ISR_WUTF;
    EXTI->PR = EXTI_PR_PR22;
    g_rtc_wakeups++;
}

/* ---- the three strategies ------------------------------------------------ */
static void busy_wait_ms(uint32_t ms)
{
    uint32_t start = millis();
    while (millis() - start < ms) {             /* no WFI: the core never rests */
    }
}

static void enter_stop_mode(void)
{
    PWR->CR &= ~PWR_CR_PDDS;                    /* deep sleep = Stop, not Standby */
    PWR->CR |= PWR_CR_LPDS;                     /* low-power regulator during Stop */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI();                                    /* any EXTI line (here: 22, RTC) wakes us */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    /* On silicon the system clock is now HSI. A PLL setup would be redone here. */
}

/*
 * Standby: the 1.2 V domain is switched off, RAM and registers are lost, and
 * waking up (WKUP pin, RTC, NRST, IWDG) is a reset. Renode does not model it,
 * so the demo never calls this.
 */
__attribute__((unused)) static void enter_standby_mode(void)
{
    PWR->CR |= PWR_CR_CWUF;                     /* clear the wakeup flag */
    PWR->CSR |= PWR_CSR_EWUP;                   /* PA0 rising edge may wake us */
    PWR->CR |= PWR_CR_PDDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI();                                    /* does not return */
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    rtc_wakeup_init_100ms();

    printf("\nlesson 21: %d jobs, one every 100 ms, three ways\n", JOBS);

    mark_phase(1);
    for (int i = 0; i < JOBS; i++) {
        busy_wait_ms(100);
        job();
    }

    mark_phase(2);
    for (int i = 0; i < JOBS; i++) {
        delay_ms(100);                          /* WFI inside: sleeps between SysTick ticks */
        job();
    }

    mark_phase(3);
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;  /* 1 kHz interrupts would end every Stop */
    RTC->ISR &= ~RTC_ISR_WUTF;
    EXTI->PR = EXTI_PR_PR22;
    RTC->CR |= RTC_CR_WUTIE | RTC_CR_WUTE;
    for (int i = 0; i < JOBS; i++) {
        enter_stop_mode();
        job();
    }
    RTC->CR &= ~RTC_CR_WUTE;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    mark_phase(4);
    printf("done: %lu RTC wakeups in phase 3\n", (unsigned long)g_rtc_wakeups);
    led_off(LED_GREEN);
    for (;;) {
        __WFI();
    }
}
