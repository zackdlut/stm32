/*
 * Lesson 10, Dikuoxing STM32F103C8T6 version: hardware timers.
 *
 *   TIM4  update interrupt every 500 ms        -> toggles the PC13 user LED
 *   TIM3  channel 3 PWM, 100 Hz, on PB0        -> external LED brightness
 *   TIM2  10 kHz update interrupt              -> samples PB0 to measure duty
 *
 * PB0 is the pin the LCSC beginner manual uses for its breathing LED: wire an
 * LED with a 1k resistor from PB0 to GND. PC13 is not a timer pin, so the
 * board LED is driven from an interrupt instead.
 *
 * Timer clock after reset: APB1 = 8 MHz and its prescaler is /1, so TIM2..4
 * count at 8 MHz. Update frequency = f_timer / ((PSC + 1) * (ARR + 1)).
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/systick.h"
#include "drivers/uart.h"

#define TIMER_CLK_HZ 8000000u
#define PWM_PIN      0u                      /* PB0 = TIM3_CH3 */

/* ---- TIM4: periodic interrupt -------------------------------------------- */
static volatile uint32_t g_tim4_ticks;

static void tim4_init_2hz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    TIM4->PSC = TIMER_CLK_HZ / 1000u - 1u;   /* 8 MHz / 8000 = 1 kHz counter */
    TIM4->ARR = 500u - 1u;                   /* 500 counts = 500 ms */
    TIM4->EGR = TIM_EGR_UG;                  /* load PSC/ARR now, not at the first overflow */
    TIM4->SR = 0;                            /* UG also set UIF: clear it */
    TIM4->DIER = TIM_DIER_UIE;
    nvic_set_priority(TIM4_IRQn, 4);
    nvic_enable_irq(TIM4_IRQn);
    TIM4->CR1 = TIM_CR1_CEN;
}

void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF) {
        TIM4->SR = ~TIM_SR_UIF;              /* rc_w0: write 0 to clear, 1 leaves others */
        g_tim4_ticks++;
        led_toggle();
    }
}

/* ---- TIM3 CH3: PWM on PB0 ------------------------------------------------ */
#define PWM_PERIOD 10000u                    /* 1 MHz / 10000 = 100 Hz */

static void tim3_pwm_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    /* "Alternate function push-pull": the pin follows TIM3_CH3, its default
     * owner. There is no AF number to pick on the F1. */
    gpio_config(GPIOB, PWM_PIN, GPIO_CFG_AF_PP_50MHZ);

    TIM3->PSC = TIMER_CLK_HZ / 1000000u - 1u;    /* 1 MHz counter: 1 count = 1 us */
    TIM3->ARR = PWM_PERIOD - 1u;
    TIM3->CCR[2] = 0;

    /* PWM mode 1: output high while CNT < CCR3, low afterwards. OC3PE buffers
     * CCR3 so a new duty takes effect at the next period boundary. Channels 3
     * and 4 are configured in CCMR2. */
    TIM3->CCMR2 = (TIM_OCMODE_PWM1 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
    TIM3->CCER = TIM_CCER_CC3E;
    TIM3->CR1 = TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void pwm_set_percent(uint32_t percent)
{
    TIM3->CCR[2] = PWM_PERIOD * percent / 100u;
}

/* ---- TIM2: 10 kHz sampler that measures the PWM duty cycle --------------- */
static volatile uint32_t g_samples;
static volatile uint32_t g_samples_high;

static void tim2_sampler_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 0;
    TIM2->ARR = TIMER_CLK_HZ / 10000u - 1u;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->DIER = TIM_DIER_UIE;
    nvic_set_priority(TIM2_IRQn, 2);
    nvic_enable_irq(TIM2_IRQn);
    TIM2->CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    TIM2->SR = ~TIM_SR_UIF;
    g_samples++;
    if (GPIOB->IDR & BIT(PWM_PIN)) {
        g_samples_high++;
    }
}

static uint32_t measure_duty_percent(uint32_t window_ms)
{
    uint32_t primask = irq_save();
    g_samples = 0;
    g_samples_high = 0;
    irq_restore(primask);

    delay_ms(window_ms);

    primask = irq_save();
    uint32_t total = g_samples;
    uint32_t high = g_samples_high;
    irq_restore(primask);

    return total ? (high * 100u + total / 2u) / total : 0;
}

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    systick_init(HSI_HZ);

    tim4_init_2hz();
    tim3_pwm_init();
    tim2_sampler_init();

    printf("TIM3 PWM: PSC=%lu ARR=%lu -> %lu Hz\n", (unsigned long)TIM3->PSC, (unsigned long)TIM3->ARR,
           (unsigned long)(TIMER_CLK_HZ / ((TIM3->PSC + 1u) * (TIM3->ARR + 1u))));

    static const uint32_t duties[] = {10, 25, 50, 75, 90};
    for (unsigned i = 0; i < sizeof duties / sizeof duties[0]; i++) {
        pwm_set_percent(duties[i]);
        delay_ms(20);                                   /* let the new CCR3 take effect */
        uint32_t measured = measure_duty_percent(200);
        printf("duty set %2lu%%  CCR3=%5lu  measured %2lu%%  (t=%lu ms)\n",
               (unsigned long)duties[i], (unsigned long)TIM3->CCR[2],
               (unsigned long)measured, (unsigned long)millis());
    }

    printf("TIM4 interrupts so far: %lu (one per 500 ms)\n", (unsigned long)g_tim4_ticks);

    /* Breathing LED: ramp the duty up and down forever. */
    printf("breathing...\n");
    for (;;) {
        for (uint32_t p = 0; p <= 100; p += 5) {
            pwm_set_percent(p);
            delay_ms(20);
        }
        for (uint32_t p = 100; p > 0; p -= 5) {
            pwm_set_percent(p);
            delay_ms(20);
        }
    }
}
