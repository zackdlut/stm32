/*
 * Lesson 20: real-time clock and independent watchdog.
 *
 *   RTC   clocked by the 32.768 kHz LSE crystal, calendar in BCD,
 *         wakeup timer every second (EXTI 22), alarm A at a set time (EXTI 17)
 *   IWDG  clocked by the ~32 kHz LSI, resets the chip unless refreshed in time
 *
 * First boot: set the clock, watch a few wakeups and the alarm, feed the
 * watchdog for a while, then "hang". The watchdog resets the chip; the second
 * boot finds out why it restarted.
 */
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

/* Kept across resets: .noinit is neither loaded nor zeroed at startup. */
#define BOOT_MAGIC 0xB007C0DEu
__attribute__((section(".noinit"))) static uint32_t g_boot_magic;
__attribute__((section(".noinit"))) static uint32_t g_boot_count;

static volatile uint32_t g_wakeups;
static volatile uint32_t g_alarm;

/* ---- BCD helpers --------------------------------------------------------- */
static uint32_t bcd(uint32_t v)     { return ((v / 10u) << 4) | (v % 10u); }
static uint32_t from_bcd(uint32_t b) { return (b >> 4) * 10u + (b & 0xFu); }

/* ---- RTC ----------------------------------------------------------------- */
static void rtc_unlock(void)
{
    RTC->WPR = 0xCA;                         /* write-protection key sequence */
    RTC->WPR = 0x53;
}

static void rtc_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;                   /* allow writes to the backup domain */

    RCC->BDCR |= RCC_BDCR_LSEON;             /* 32.768 kHz crystal */
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {
    }
    RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) | RCC_BDCR_RTCSEL_0;   /* RTCCLK = LSE */
    RCC->BDCR |= RCC_BDCR_RTCEN;

    rtc_unlock();
    RTC->ISR |= RTC_ISR_INIT;                /* stop the calendar to set it */
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {
    }
    /* 32768 Hz / (127 + 1) / (255 + 1) = 1 Hz calendar tick */
    RTC->PRER = (127u << RTC_PRER_PREDIV_A_Pos) | (255u << RTC_PRER_PREDIV_S_Pos);
    RTC->TR = (bcd(22) << RTC_TR_HU_Pos) | (bcd(0) << RTC_TR_MNU_Pos) | (bcd(0) << RTC_TR_SU_Pos);
    RTC->DR = (bcd(26) << RTC_DR_YU_Pos) | (bcd(10) << RTC_DR_MU_Pos) | (bcd(23) << RTC_DR_DU_Pos) |
              (5u << RTC_DR_WDU_Pos);        /* 2026-10-23, Friday (1 = Monday) */
    RTC->CR &= ~RTC_CR_FMT;                  /* 24-hour format */
    RTC->ISR &= ~RTC_ISR_INIT;               /* run */
}

static void rtc_wakeup_every_second(void)
{
    RTC->CR &= ~RTC_CR_WUTE;
    while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {   /* wait until WUTR may be written */
    }
    RTC->WUTR = 0;                           /* ck_spre (1 Hz) / (0 + 1) */
    RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL) | RTC_CR_WUCKSEL_2;   /* 10x: 1 Hz clock */
    RTC->CR |= RTC_CR_WUTIE | RTC_CR_WUTE;

    EXTI->IMR |= EXTI_IMR_MR22;              /* the RTC reaches the NVIC through EXTI */
    EXTI->RTSR |= EXTI_RTSR_TR22;
    NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

static void rtc_alarm_at(uint32_t h, uint32_t m, uint32_t s)
{
    RTC->CR &= ~RTC_CR_ALRAE;
    while ((RTC->ISR & RTC_ISR_ALRAWF) == 0) {
    }
    /* MSK4 = 1: ignore the date, match hours, minutes and seconds. */
    RTC->ALRMAR = RTC_ALRMAR_MSK4 | (bcd(h) << RTC_ALRMAR_HU_Pos) |
                  (bcd(m) << RTC_ALRMAR_MNU_Pos) | (bcd(s) << RTC_ALRMAR_SU_Pos);
    RTC->CR |= RTC_CR_ALRAIE | RTC_CR_ALRAE;

    EXTI->IMR |= EXTI_IMR_MR17;
    EXTI->RTSR |= EXTI_RTSR_TR17;
    NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

static void rtc_print_now(void)
{
    uint32_t tr = RTC->TR;                   /* reading TR locks DR until DR is read */
    uint32_t dr = RTC->DR;
    printf("20%02lu-%02lu-%02lu %02lu:%02lu:%02lu",
           (unsigned long)from_bcd((dr >> RTC_DR_YU_Pos) & 0xFF), (unsigned long)from_bcd((dr >> RTC_DR_MU_Pos) & 0x1F),
           (unsigned long)from_bcd(dr & 0x3F), (unsigned long)from_bcd((tr >> RTC_TR_HU_Pos) & 0x3F),
           (unsigned long)from_bcd((tr >> RTC_TR_MNU_Pos) & 0x7F), (unsigned long)from_bcd(tr & 0x7F));
}

void RTC_WKUP_IRQHandler(void)
{
    RTC->ISR &= ~RTC_ISR_WUTF;               /* clear in the RTC ... */
    EXTI->PR = EXTI_PR_PR22;                 /* ... and in EXTI */
    g_wakeups++;
}

void RTC_Alarm_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_ALRAF) {
        RTC->ISR &= ~RTC_ISR_ALRAF;
        g_alarm = 1;
    }
    EXTI->PR = EXTI_PR_PR17;
}

/* ---- IWDG ---------------------------------------------------------------- */
static void iwdg_start(uint32_t timeout_ms)
{
    IWDG->KR = 0xCCCC;                       /* start: from now on it cannot be stopped */
    IWDG->KR = 0x5555;                       /* unlock PR and RLR */
    IWDG->PR = 3;                            /* LSI / 32 -> 1 kHz, 1 count = 1 ms */
    IWDG->RLR = timeout_ms - 1u;             /* 12 bits: up to 4096 ms at this prescaler */
    while (IWDG->SR != 0) {                  /* wait for the values to reach the LSI domain */
    }
    IWDG->KR = 0xAAAA;                       /* reload the counter */
}

static void iwdg_feed(void) { IWDG->KR = 0xAAAA; }

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);

    uint32_t csr = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;                /* clear the reset flags for next time */
    if (g_boot_magic != BOOT_MAGIC) {
        g_boot_magic = BOOT_MAGIC;
        g_boot_count = 0;
    }
    g_boot_count++;

    printf("\nlesson 20, boot #%lu, RCC_CSR = 0x%08lx%s%s%s\n", (unsigned long)g_boot_count,
           (unsigned long)csr, (csr & RCC_CSR_IWDGRSTF) ? " IWDG" : "", (csr & RCC_CSR_PORRSTF) ? " POR" : "",
           (csr & RCC_CSR_PINRSTF) ? " PIN" : "");

    if (g_boot_count > 1) {
        printf("restarted after boot #%lu stopped feeding the watchdog\n", (unsigned long)(g_boot_count - 1));
        led_on(LED_RED);
        for (;;) {
            __WFI();
        }
    }

    rtc_init();
    printf("RTC set to ");
    rtc_print_now();
    printf("\n");

    rtc_alarm_at(22, 0, 3);
    rtc_wakeup_every_second();

    uint32_t seen = 0;
    while (g_wakeups < 4) {
        __WFI();
        if (g_wakeups != seen) {
            seen = g_wakeups;
            printf("wakeup %lu: ", (unsigned long)seen);
            rtc_print_now();
            printf("%s\n", g_alarm ? "  <- alarm A fired" : "");
            g_alarm = 0;
            led_toggle(LED_BLUE);
        }
    }
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_ALRAE);

    iwdg_start(500);
    printf("IWDG started, timeout 500 ms\n");
    for (int i = 0; i < 5; i++) {
        delay_ms(300);
        iwdg_feed();
        printf("fed the watchdog at t=%lu ms\n", (unsigned long)millis());
    }

    printf("simulating a hang: no more feeding\n");
    uint32_t hang_start = millis();
    for (;;) {
        delay_ms(100);
        printf("  still alive %lu ms after the last feed\n", (unsigned long)(millis() - hang_start));
    }
}
