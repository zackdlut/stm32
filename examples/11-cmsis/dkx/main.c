/*
 * Lesson 11, Dikuoxing STM32F103C8T6 version: the official CMSIS headers.
 *
 * Same hardware as lessons 6-8 (SysTick, button interrupt, UART, LED), but
 * every register name, bit mask and core helper now comes from:
 *   stm32f1xx.h  -> stm32f103xb.h   ST's device header (C8 and CB share it)
 *   core_cm3.h                      ARM's Cortex-M3 header: NVIC, SysTick, SCB, DWT
 *   system_stm32f1xx.c              SystemInit(), SystemCoreClock
 * Startup is ST's startup_stm32f103xb.s instead of our startup.c.
 */
#include <stdio.h>

#include "stm32f1xx.h"

static volatile uint32_t g_ticks;
static volatile uint32_t g_presses;

void SysTick_Handler(void)
{
    g_ticks++;
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0) {
        EXTI->PR = EXTI_PR_PR0;
        g_presses++;
    }
}

int __io_putchar(int ch)
{
    if (ch == '\n') {
        __io_putchar('\r');
    }
    while ((USART1->SR & USART_SR_TXE) == 0) {
    }
    USART1->DR = (uint8_t)ch;
    return ch;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = g_ticks;
    while (g_ticks - start < ms) {
        __WFI();
    }
}

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    /* PC13 output, 2 MHz push-pull: MODE13 = 10, CNF13 = 00. Each pin has a
     * MODE and a CNF field of two bits; _0 / _1 are the low and high bit. */
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13)) | GPIO_CRH_MODE13_1;

    /* PA0 input with pull-down: MODE0 = 00, CNF0 = 10, ODR0 = 0. */
    GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0)) | GPIO_CRL_CNF0_1;
    GPIOA->BRR = GPIO_BRR_BR0;

    /* PA9 AF push-pull 50 MHz (USART1_TX), PA10 input with pull-up (USART1_RX). */
    GPIOA->CRH = (GPIOA->CRH & ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9 | GPIO_CRH_MODE10 | GPIO_CRH_CNF10)) |
                 GPIO_CRH_MODE9 | GPIO_CRH_CNF9_1 | GPIO_CRH_CNF10_1;
    GPIOA->BSRR = GPIO_BSRR_BS10;
}

static void uart_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = (SystemCoreClock + 115200u / 2u) / 115200u;   /* APB2 /1 after reset */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void button_irq_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~AFIO_EXTICR1_EXTI0) | AFIO_EXTICR1_EXTI0_PA;
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;

    /* CMSIS NVIC API: PRIGROUP = 3 -> all 4 priority bits preempt, 0 bits sub
     * (HAL calls this NVIC_PRIORITYGROUP_4). */
    NVIC_SetPriorityGrouping(3);
    NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* DWT->CYCCNT counts core clock cycles: the standard way to time code. */
static void cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t sum_to(uint32_t n)
{
    uint32_t s = 0;
    for (volatile uint32_t i = 1; i <= n; i++) {
        s += i;
    }
    return s;
}

int main(void)
{
    /* SystemInit() already ran from the startup file (clocks at reset state). */
    SystemCoreClockUpdate();
    gpio_init();
    uart_init();
    SysTick_Config(SystemCoreClock / 1000u);   /* CMSIS: LOAD, VAL, CTRL and priority */
    button_irq_init();
    cycle_counter_init();

    printf("\nCMSIS: SystemCoreClock = %lu Hz\n", (unsigned long)SystemCoreClock);
    printf("CPUID = 0x%08lx, implementer 0x%02lx, part 0x%03lx\n",
           (unsigned long)SCB->CPUID,
           (unsigned long)((SCB->CPUID & SCB_CPUID_IMPLEMENTER_Msk) >> SCB_CPUID_IMPLEMENTER_Pos),
           (unsigned long)((SCB->CPUID & SCB_CPUID_PARTNO_Msk) >> SCB_CPUID_PARTNO_Pos));
    printf("SysTick LOAD = %lu, EXTI0 priority = %lu\n",
           (unsigned long)SysTick->LOAD, (unsigned long)NVIC_GetPriority(EXTI0_IRQn));

    uint32_t start = DWT->CYCCNT;
    uint32_t s = sum_to(1000);
    uint32_t cycles = DWT->CYCCNT - start;
    printf("sum_to(1000) = %lu in %lu cycles%s\n", (unsigned long)s, (unsigned long)cycles,
           cycles == 0 ? " (DWT is not modelled by Renode; real silicon counts here)" : "");

    printf("__RBIT(1) = 0x%08lx, __REV(0x11223344) = 0x%08lx, __CLZ(0x00010000) = %lu\n",
           (unsigned long)__RBIT(1u), (unsigned long)__REV(0x11223344u), (unsigned long)__CLZ(0x00010000u));

    for (int i = 0; i < 6; i++) {
        /* One BSRR write either sets or resets PC13: no read-modify-write. */
        GPIOC->BSRR = (i & 1) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
        delay_ms(100);
    }

    printf("button presses seen by EXTI0: %lu (t=%lu ms)\n", (unsigned long)g_presses, (unsigned long)g_ticks);
    for (;;) {
        __WFI();
    }
}
