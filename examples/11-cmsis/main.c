/*
 * Lesson 11: the official CMSIS headers.
 *
 * Same hardware as lessons 6-8 (SysTick, button interrupt, UART, LEDs), but
 * every register name, bit mask and core helper now comes from:
 *   stm32f4xx.h  -> stm32f407xx.h   ST's device header (generated from the SVD)
 *   core_cm4.h                      ARM's Cortex-M4 header: NVIC, SysTick, SCB, DWT
 *   system_stm32f4xx.c              SystemInit(), SystemCoreClock
 * Startup is ST's startup_stm32f407xx.s instead of our startup.c.
 */
#include <stdio.h>

#include "stm32f4xx.h"

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
        GPIOD->ODR ^= GPIO_ODR_OD15;
    }
}

int __io_putchar(int ch)
{
    if (ch == '\n') {
        __io_putchar('\r');
    }
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
    USART2->DR = (uint8_t)ch;
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
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIODEN;

    /* PD12..PD15 outputs: MODERy = 01. The _0 / _1 suffixes are the two bits. */
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 |
                    GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;

    /* PA0 input with pull-down. */
    GPIOA->MODER &= ~GPIO_MODER_MODER0;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~GPIO_PUPDR_PUPD0) | GPIO_PUPDR_PUPD0_1;

    /* PA2/PA3 alternate function 7 (USART2). */
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3)) |
                   GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3)) |
                    (7u << GPIO_AFRL_AFSEL2_Pos) | (7u << GPIO_AFRL_AFSEL3_Pos);
}

static void uart_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->BRR = (SystemCoreClock + 115200u / 2u) / 115200u;   /* APB1 /1 after reset */
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void button_irq_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~SYSCFG_EXTICR1_EXTI0) | SYSCFG_EXTICR1_EXTI0_PA;
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
    /* SystemInit() already ran from the startup file (FPU on, clocks at reset state). */
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
        /* Reset PD12..14 and set one of them in a single write: when both the
         * set and reset bit of a pin are 1, BSRR gives the set bit priority.
         * PD15 (blue) belongs to the button interrupt. */
        GPIOD->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR13 | GPIO_BSRR_BR14 |
                      (GPIO_BSRR_BS12 << (i % 3));
        delay_ms(100);
    }

    printf("button presses seen by EXTI0: %lu (t=%lu ms)\n", (unsigned long)g_presses, (unsigned long)g_ticks);
    for (;;) {
        __WFI();
    }
}
