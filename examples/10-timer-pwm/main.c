/*
 * Lesson 10: hardware timers.
 *
 *   TIM3  update interrupt every 500 ms        -> toggles the blue LED
 *   TIM4  channel 1 PWM, 100 Hz, on PD12 (AF2) -> green LED brightness
 *   TIM2  10 kHz update interrupt              -> samples PD12 to measure duty
 *
 * Timer clock after reset: APB1 = 16 MHz and its prescaler is /1, so TIM2..4
 * count at 16 MHz. Update frequency = f_timer / ((PSC + 1) * (ARR + 1)).
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/systick.h"
#include "drivers/uart.h"

#define TIMER_CLK_HZ 16000000u
#define GPIO_AF2_TIM4 2u

/* ---- TIM3: periodic interrupt -------------------------------------------- */
static volatile uint32_t g_tim3_ticks;

static void tim3_init_2hz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = TIMER_CLK_HZ / 1000u - 1u;   /* 16 MHz / 16000 = 1 kHz counter */
    TIM3->ARR = 500u - 1u;                   /* 500 counts = 500 ms */
    TIM3->EGR = TIM_EGR_UG;                  /* load PSC/ARR now, not at the first overflow */
    TIM3->SR = 0;                            /* UG also set UIF: clear it */
    TIM3->DIER = TIM_DIER_UIE;
    nvic_set_priority(TIM3_IRQn, 4);
    nvic_enable_irq(TIM3_IRQn);
    TIM3->CR1 = TIM_CR1_CEN;
}

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR = ~TIM_SR_UIF;              /* rc_w0: write 0 to clear, 1 leaves others */
        g_tim3_ticks++;
        led_toggle(LED_BLUE);
    }
}

/* ---- TIM4 CH1: PWM on PD12 ----------------------------------------------- */
#define PWM_PERIOD 10000u                    /* 1 MHz / 10000 = 100 Hz */

static void tim4_pwm_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    gpio_set_af(GPIOD, LED_GREEN, GPIO_AF2_TIM4);

    TIM4->PSC = TIMER_CLK_HZ / 1000000u - 1u;    /* 1 MHz counter: 1 count = 1 us */
    TIM4->ARR = PWM_PERIOD - 1u;
    TIM4->CCR[0] = 0;

    /* PWM mode 1: output high while CNT < CCR1, low afterwards. OC1PE buffers
     * CCR1 so a new duty takes effect at the next period boundary. */
    TIM4->CCMR1 = (TIM_OCMODE_PWM1 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM4->CCER = TIM_CCER_CC1E;
    TIM4->CR1 = TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;
}

static void pwm_set_percent(uint32_t percent)
{
    TIM4->CCR[0] = PWM_PERIOD * percent / 100u;
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
    if (GPIOD->IDR & BIT(LED_GREEN)) {
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
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    systick_init(HSI_HZ);

    tim3_init_2hz();
    tim4_pwm_init();
    tim2_sampler_init();

    printf("TIM4 PWM: PSC=%lu ARR=%lu -> %lu Hz\n", (unsigned long)TIM4->PSC, (unsigned long)TIM4->ARR,
           (unsigned long)(TIMER_CLK_HZ / ((TIM4->PSC + 1u) * (TIM4->ARR + 1u))));

    static const uint32_t duties[] = {10, 25, 50, 75, 90};
    for (unsigned i = 0; i < sizeof duties / sizeof duties[0]; i++) {
        pwm_set_percent(duties[i]);
        delay_ms(20);                                   /* let the new CCR1 take effect */
        uint32_t measured = measure_duty_percent(200);
        printf("duty set %2lu%%  CCR1=%5lu  measured %2lu%%  (t=%lu ms)\n",
               (unsigned long)duties[i], (unsigned long)TIM4->CCR[0],
               (unsigned long)measured, (unsigned long)millis());
    }

    printf("TIM3 interrupts so far: %lu (one per 500 ms)\n", (unsigned long)g_tim3_ticks);

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
