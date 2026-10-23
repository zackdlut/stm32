/*
 * Lesson 19: analog in and out.
 *
 *   ADC1 IN1 (PA1)   single conversions, polled
 *   ADC1 IN1 + IN8 (PB0)  scan sequence, results moved by DMA2 stream 0
 *   DAC channel 1 (PA4)   set a few output voltages
 *
 * stimulus.resc feeds sample values into the simulated ADC.
 */
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define VREF_MV 3300u

static uint32_t raw_to_mv(uint32_t raw) { return (raw * VREF_MV + 2047u) / 4095u; }

/* ---- ADC ----------------------------------------------------------------- */
static void adc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    gpio_set_mode(GPIOA, 1, GPIO_MODE_ANALOG);   /* analog mode disconnects the digital input */
    gpio_set_mode(GPIOB, 0, GPIO_MODE_ANALOG);

    ADC->CCR = ADC_CCR_ADCPRE_0;                  /* ADCCLK = PCLK2 / 4 = 4 MHz (max 36 MHz) */
    ADC1->CR1 = 0;                                /* 12-bit resolution */
    ADC1->CR2 = 0;
    /* Sampling time 84 cycles for IN1 and IN8: longer suits high source impedance. */
    ADC1->SMPR2 = (4u << ADC_SMPR2_SMP1_Pos) | (4u << ADC_SMPR2_SMP8_Pos);
    ADC1->SQR1 = 0;                               /* configure the sequence, then power up */
    ADC1->SQR3 = 1;
    ADC1->CR2 |= ADC_CR2_ADON;
}

static uint32_t adc_read_single(uint32_t channel)
{
    ADC1->SQR1 = 0;                               /* L = 0: one conversion */
    ADC1->SQR3 = channel;                         /* SQ1 = channel */
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {
    }
    return ADC1->DR;                              /* reading DR clears EOC */
}

/* ---- ADC scan + DMA ------------------------------------------------------ */
#define SCAN_CHANNELS 2u
#define SCAN_ROUNDS   4u
static volatile uint32_t g_samples[SCAN_ROUNDS * SCAN_CHANNELS];
static volatile uint32_t g_block_done;

void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
        ADC1->CR2 &= ~(ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS);   /* block captured: stop */
        g_block_done = 1;
    }
}

static void adc_scan_dma_start(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    DMA_Stream_TypeDef *s = DMA2_Stream0;
    s->CR = 0;
    while (s->CR & DMA_SxCR_EN) {
    }
    s->PAR = (uint32_t)&ADC1->DR;
    s->M0AR = (uint32_t)g_samples;
    s->NDTR = SCAN_ROUNDS * SCAN_CHANNELS;
    s->CR = (0u << DMA_SxCR_CHSEL_Pos)            /* channel 0 = ADC1 */
          | DMA_SxCR_MINC                         /* peripheral -> memory (DIR = 00) */
          | DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1   /* 32-bit items: whole DR register */
          | DMA_SxCR_TCIE;
    NVIC_SetPriority(DMA2_Stream0_IRQn, 5);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    s->CR |= DMA_SxCR_EN;

    ADC1->SQR1 = (SCAN_CHANNELS - 1u) << ADC_SQR1_L_Pos;   /* L = number of conversions - 1 */
    ADC1->SQR3 = (1u << ADC_SQR3_SQ1_Pos) | (8u << ADC_SQR3_SQ2_Pos);
    ADC1->CR1 |= ADC_CR1_SCAN;                    /* convert the whole sequence */
    ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS        /* one DMA request per conversion */
               | ADC_CR2_CONT;                    /* restart the sequence when it ends */
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

/* ---- DAC ----------------------------------------------------------------- */
static void dac_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    gpio_set_mode(GPIOA, 4, GPIO_MODE_ANALOG);    /* PA4 = DAC_OUT1 */
    DAC->CR = DAC_CR_EN1;                         /* no trigger: DHR -> DOR right away */
}

static void dac_set_mv(uint32_t mv)
{
    DAC->DHR12R1 = (mv * 4095u + VREF_MV / 2u) / VREF_MV;
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    adc_init();

    printf("\nlesson 19: ADC1 12-bit, Vref = %lu mV\n", (unsigned long)VREF_MV);

    for (int i = 0; i < 3; i++) {
        uint32_t raw = adc_read_single(1);
        printf("single IN1: raw %4lu -> %4lu mV  (t=%lu ms)\n", (unsigned long)raw,
               (unsigned long)raw_to_mv(raw), (unsigned long)millis());
        delay_ms(50);
    }

    adc_scan_dma_start();
    while (!g_block_done) {
        __WFI();
    }
    printf("scan IN1, IN8 x %u rounds by DMA: done, NDTR = %lu\n", (unsigned)SCAN_ROUNDS,
           (unsigned long)DMA2_Stream0->NDTR);
    printf("buffer:");
    for (uint32_t i = 0; i < SCAN_ROUNDS * SCAN_CHANNELS; i++) {
        printf(" %s%4lu", i % 2 == 0 ? "IN1=" : "IN8=", (unsigned long)g_samples[i]);
    }
    printf("\n");

    dac_init();
    static const uint32_t levels_mv[] = {0, 825, 1650, 2475, 3300};
    for (unsigned i = 0; i < sizeof levels_mv / sizeof levels_mv[0]; i++) {
        dac_set_mv(levels_mv[i]);
        printf("DAC: request %4lu mV -> DHR12R1 = %4lu, DOR1 = %4lu\n", (unsigned long)levels_mv[i],
               (unsigned long)DAC->DHR12R1, (unsigned long)DAC->DOR1);
    }

    led_on(LED_GREEN);
    for (;;) {
        __WFI();
    }
}
