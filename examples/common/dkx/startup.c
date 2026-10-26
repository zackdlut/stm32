/*
 * Startup code for STM32F103 (medium density: C8/CB), written in C.
 *
 * Identical in structure to ../startup.c: the Cortex-M3 loads the stack
 * pointer from word 0 of the vector table and jumps to Reset_Handler in word 1.
 * Only the peripheral interrupt list differs (43 vectors instead of 82), and
 * there is no FPU to switch on.
 */
#include <stdint.h>

#include "stm32f103.h"

/* Symbols defined by the linker script. Only their addresses are meaningful. */
extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;

extern int main(void);
extern void __libc_init_array(void);

void Reset_Handler(void);
void Default_Handler(void);

#define WEAK_HANDLER(name) void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_HANDLER(NMI_Handler);
WEAK_HANDLER(HardFault_Handler);
WEAK_HANDLER(MemManage_Handler);
WEAK_HANDLER(BusFault_Handler);
WEAK_HANDLER(UsageFault_Handler);
WEAK_HANDLER(SVC_Handler);
WEAK_HANDLER(DebugMon_Handler);
WEAK_HANDLER(PendSV_Handler);
WEAK_HANDLER(SysTick_Handler);

WEAK_HANDLER(WWDG_IRQHandler);
WEAK_HANDLER(PVD_IRQHandler);
WEAK_HANDLER(TAMPER_IRQHandler);
WEAK_HANDLER(RTC_IRQHandler);
WEAK_HANDLER(FLASH_IRQHandler);
WEAK_HANDLER(RCC_IRQHandler);
WEAK_HANDLER(EXTI0_IRQHandler);
WEAK_HANDLER(EXTI1_IRQHandler);
WEAK_HANDLER(EXTI2_IRQHandler);
WEAK_HANDLER(EXTI3_IRQHandler);
WEAK_HANDLER(EXTI4_IRQHandler);
WEAK_HANDLER(DMA1_Channel1_IRQHandler);
WEAK_HANDLER(DMA1_Channel2_IRQHandler);
WEAK_HANDLER(DMA1_Channel3_IRQHandler);
WEAK_HANDLER(DMA1_Channel4_IRQHandler);
WEAK_HANDLER(DMA1_Channel5_IRQHandler);
WEAK_HANDLER(DMA1_Channel6_IRQHandler);
WEAK_HANDLER(DMA1_Channel7_IRQHandler);
WEAK_HANDLER(ADC1_2_IRQHandler);
WEAK_HANDLER(USB_HP_CAN1_TX_IRQHandler);
WEAK_HANDLER(USB_LP_CAN1_RX0_IRQHandler);
WEAK_HANDLER(CAN1_RX1_IRQHandler);
WEAK_HANDLER(CAN1_SCE_IRQHandler);
WEAK_HANDLER(EXTI9_5_IRQHandler);
WEAK_HANDLER(TIM1_BRK_IRQHandler);
WEAK_HANDLER(TIM1_UP_IRQHandler);
WEAK_HANDLER(TIM1_TRG_COM_IRQHandler);
WEAK_HANDLER(TIM1_CC_IRQHandler);
WEAK_HANDLER(TIM2_IRQHandler);
WEAK_HANDLER(TIM3_IRQHandler);
WEAK_HANDLER(TIM4_IRQHandler);
WEAK_HANDLER(I2C1_EV_IRQHandler);
WEAK_HANDLER(I2C1_ER_IRQHandler);
WEAK_HANDLER(I2C2_EV_IRQHandler);
WEAK_HANDLER(I2C2_ER_IRQHandler);
WEAK_HANDLER(SPI1_IRQHandler);
WEAK_HANDLER(SPI2_IRQHandler);
WEAK_HANDLER(USART1_IRQHandler);
WEAK_HANDLER(USART2_IRQHandler);
WEAK_HANDLER(USART3_IRQHandler);
WEAK_HANDLER(EXTI15_10_IRQHandler);
WEAK_HANDLER(RTC_Alarm_IRQHandler);
WEAK_HANDLER(USBWakeUp_IRQHandler);

typedef void (*vector_t)(void);

/* Initial SP, 15 system exception vectors, 43 peripheral interrupt vectors. */
__attribute__((section(".isr_vector"), used))
const vector_t g_vector_table[16 + 43] = {
    (vector_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,                /* 0 */
    PVD_IRQHandler,
    TAMPER_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,               /* 6 */
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,               /* 10 */
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_2_IRQHandler,
    USB_HP_CAN1_TX_IRQHandler,
    USB_LP_CAN1_RX0_IRQHandler,     /* 20 */
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_IRQHandler,
    TIM1_UP_IRQHandler,
    TIM1_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,                /* 28 */
    TIM3_IRQHandler,
    TIM4_IRQHandler,                /* 30 */
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,              /* 37 */
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,           /* 40 */
    RTC_Alarm_IRQHandler,
    USBWakeUp_IRQHandler,
};

/* Weak so a lesson can supply its own clock setup. */
__attribute__((weak)) void SystemInit(void)
{
}

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) {
        *dst++ = *src++;
    }

    for (dst = &_sbss; dst < &_ebss;) {
        *dst++ = 0;
    }

    SystemInit();
    __libc_init_array();

    main();

    for (;;) {
    }
}

void Default_Handler(void)
{
    for (;;) {
    }
}
