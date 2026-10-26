/*
 * Lesson 19, Dikuoxing STM32F103C8T6 version: analog in.
 *
 *   ADC1 IN5  (PA5)  external voltage, e.g. a potentiometer between 3.3 V and GND
 *   ADC1 IN16        internal temperature sensor
 *   ADC1 IN17        internal reference Vrefint (about 1.20 V)
 *
 * Differences from the F407 version: the F1 ADC must be calibrated after
 * power-up, a software start needs EXTSEL = SWSTART plus EXTTRIG, and the
 * STM32F103C8 has no DAC. Vrefint is used to measure the real supply voltage
 * instead of assuming 3.3 V.
 *
 * dkx/stimulus.resc changes the simulated IN5 voltage between readings.
 */
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define VREFINT_MV     1200u   /* datasheet typical, 1.16 V to 1.24 V */
#define TS_V25_MV      1430u   /* sensor voltage at 25 C, typical */
#define TS_SLOPE_UV_C  4300u   /* 4.3 mV per degree C, typical */

/* ---- ADC ----------------------------------------------------------------- */
static void adc_init(void)
{
    /* ADCCLK = PCLK2 / 2 = 4 MHz (max 14 MHz; at 72 MHz use /6). */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV2;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    gpio_config(GPIOA, 5, GPIO_CFG_ANALOG);       /* analog mode disconnects the digital input */

    /* Sampling time 239.5 cycles everywhere: the temperature sensor needs at
     * least 17.1 us, and long sampling suits high source impedance. */
    ADC1->SMPR2 = ADC_SMPR2_SMP5;
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
    ADC1->SQR1 = 0;                               /* L = 0: one conversion */

    /* Configure CR2 while the ADC is still off: with ADON already set, a
     * write that sets ADON again would itself start a conversion. */
    ADC1->CR2 = ADC_CR2_EXTSEL                    /* EXTSEL = 111: SWSTART bit */
              | ADC_CR2_EXTTRIG                   /* regular group trigger enabled */
              | ADC_CR2_TSVREFE;                  /* connect IN16 and IN17 */
    ADC1->CR2 |= ADC_CR2_ADON;                    /* power up */
    delay_ms(1);                                  /* tSTAB, 1 us is enough */

    /* Calibration: reset the calibration registers, then calibrate. Both
     * bits clear themselves when done. */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) {
    }
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) {
    }
}

static uint32_t adc_read_single(uint32_t channel)
{
    ADC1->SQR3 = channel;                         /* SQ1 = channel */
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {
    }
    return ADC1->DR;                              /* reading DR clears EOC */
}

static uint32_t raw_to_mv(uint32_t raw, uint32_t vdda_mv) { return (raw * vdda_mv + 2047u) / 4095u; }

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    adc_init();

    printf("\nlesson 19: ADC1 12-bit, ADCCLK %lu Hz, calibrated\n", (unsigned long)(HSI_HZ / 2u));

    /* Vrefint is a fixed voltage measured against VDDA, so it tells us VDDA. */
    uint32_t vref_raw = adc_read_single(17);
    uint32_t vdda_mv = (VREFINT_MV * 4095u + vref_raw / 2u) / vref_raw;
    printf("Vrefint IN17: raw %4lu -> VDDA = %lu mV\n", (unsigned long)vref_raw, (unsigned long)vdda_mv);

    /* T = (V25 - Vsense) / Avg_Slope + 25. Without factory calibration data
     * on the F1 this is accurate to a few degrees only. */
    uint32_t ts_raw = adc_read_single(16);
    int32_t ts_mv = (int32_t)raw_to_mv(ts_raw, vdda_mv);
    int32_t t_deci = ((int32_t)TS_V25_MV - ts_mv) * 10000 / (int32_t)TS_SLOPE_UV_C + 250;
    int32_t t_mag = t_deci < 0 ? -t_deci : t_deci;
    printf("sensor  IN16: raw %4lu -> %4ld mV -> %s%ld.%ld C\n", (unsigned long)ts_raw, (long)ts_mv,
           t_deci < 0 ? "-" : "", (long)(t_mag / 10), (long)(t_mag % 10));

    for (int i = 0; i < 3; i++) {
        uint32_t raw = adc_read_single(5);
        printf("single  IN5:  raw %4lu -> %4lu mV  (t=%lu ms)\n", (unsigned long)raw,
               (unsigned long)raw_to_mv(raw, vdda_mv), (unsigned long)millis());
        delay_ms(50);
    }

    led_on();
    for (;;) {
        __WFI();
    }
}
