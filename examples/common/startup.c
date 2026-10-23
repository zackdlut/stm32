/*
 * Startup code for STM32F407, written in C.
 *
 * After reset the Cortex-M4 loads the stack pointer from word 0 of the vector
 * table and jumps to the address in word 1 (Reset_Handler). Nothing else has
 * been initialised yet: .data still lives only in FLASH and .bss holds garbage.
 */
#include <stdint.h>

#include "stm32f407.h"

/* Symbols defined by stm32f407.ld. Only their addresses are meaningful. */
extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;

extern int main(void);
extern void __libc_init_array(void);

void Reset_Handler(void);
void Default_Handler(void);

/*
 * Every handler is a weak alias of Default_Handler. Defining a function with
 * the same name anywhere in the program (e.g. `void SysTick_Handler(void)`)
 * replaces the alias at link time.
 */
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
WEAK_HANDLER(TAMP_STAMP_IRQHandler);
WEAK_HANDLER(RTC_WKUP_IRQHandler);
WEAK_HANDLER(FLASH_IRQHandler);
WEAK_HANDLER(RCC_IRQHandler);
WEAK_HANDLER(EXTI0_IRQHandler);
WEAK_HANDLER(EXTI1_IRQHandler);
WEAK_HANDLER(EXTI2_IRQHandler);
WEAK_HANDLER(EXTI3_IRQHandler);
WEAK_HANDLER(EXTI4_IRQHandler);
WEAK_HANDLER(DMA1_Stream0_IRQHandler);
WEAK_HANDLER(DMA1_Stream1_IRQHandler);
WEAK_HANDLER(DMA1_Stream2_IRQHandler);
WEAK_HANDLER(DMA1_Stream3_IRQHandler);
WEAK_HANDLER(DMA1_Stream4_IRQHandler);
WEAK_HANDLER(DMA1_Stream5_IRQHandler);
WEAK_HANDLER(DMA1_Stream6_IRQHandler);
WEAK_HANDLER(ADC_IRQHandler);
WEAK_HANDLER(CAN1_TX_IRQHandler);
WEAK_HANDLER(CAN1_RX0_IRQHandler);
WEAK_HANDLER(CAN1_RX1_IRQHandler);
WEAK_HANDLER(CAN1_SCE_IRQHandler);
WEAK_HANDLER(EXTI9_5_IRQHandler);
WEAK_HANDLER(TIM1_BRK_TIM9_IRQHandler);
WEAK_HANDLER(TIM1_UP_TIM10_IRQHandler);
WEAK_HANDLER(TIM1_TRG_COM_TIM11_IRQHandler);
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
WEAK_HANDLER(OTG_FS_WKUP_IRQHandler);
WEAK_HANDLER(TIM8_BRK_TIM12_IRQHandler);
WEAK_HANDLER(TIM8_UP_TIM13_IRQHandler);
WEAK_HANDLER(TIM8_TRG_COM_TIM14_IRQHandler);
WEAK_HANDLER(TIM8_CC_IRQHandler);
WEAK_HANDLER(DMA1_Stream7_IRQHandler);
WEAK_HANDLER(FSMC_IRQHandler);
WEAK_HANDLER(SDIO_IRQHandler);
WEAK_HANDLER(TIM5_IRQHandler);
WEAK_HANDLER(SPI3_IRQHandler);
WEAK_HANDLER(UART4_IRQHandler);
WEAK_HANDLER(UART5_IRQHandler);
WEAK_HANDLER(TIM6_DAC_IRQHandler);
WEAK_HANDLER(TIM7_IRQHandler);
WEAK_HANDLER(DMA2_Stream0_IRQHandler);
WEAK_HANDLER(DMA2_Stream1_IRQHandler);
WEAK_HANDLER(DMA2_Stream2_IRQHandler);
WEAK_HANDLER(DMA2_Stream3_IRQHandler);
WEAK_HANDLER(DMA2_Stream4_IRQHandler);
WEAK_HANDLER(ETH_IRQHandler);
WEAK_HANDLER(ETH_WKUP_IRQHandler);
WEAK_HANDLER(CAN2_TX_IRQHandler);
WEAK_HANDLER(CAN2_RX0_IRQHandler);
WEAK_HANDLER(CAN2_RX1_IRQHandler);
WEAK_HANDLER(CAN2_SCE_IRQHandler);
WEAK_HANDLER(OTG_FS_IRQHandler);
WEAK_HANDLER(DMA2_Stream5_IRQHandler);
WEAK_HANDLER(DMA2_Stream6_IRQHandler);
WEAK_HANDLER(DMA2_Stream7_IRQHandler);
WEAK_HANDLER(USART6_IRQHandler);
WEAK_HANDLER(I2C3_EV_IRQHandler);
WEAK_HANDLER(I2C3_ER_IRQHandler);
WEAK_HANDLER(OTG_HS_EP1_OUT_IRQHandler);
WEAK_HANDLER(OTG_HS_EP1_IN_IRQHandler);
WEAK_HANDLER(OTG_HS_WKUP_IRQHandler);
WEAK_HANDLER(OTG_HS_IRQHandler);
WEAK_HANDLER(DCMI_IRQHandler);
WEAK_HANDLER(HASH_RNG_IRQHandler);
WEAK_HANDLER(FPU_IRQHandler);

typedef void (*vector_t)(void);

/*
 * The vector table: initial SP, then 15 system exception vectors, then 82
 * peripheral interrupt vectors. The linker script places .isr_vector at
 * 0x08000000, which is aliased to address 0 at boot.
 */
__attribute__((section(".isr_vector"), used))
const vector_t g_vector_table[16 + 82] = {
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
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,               /* 6 */
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,               /* 10 */
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,            /* 20 */
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
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
    USART1_IRQHandler,
    USART2_IRQHandler,              /* 38 */
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,           /* 40 */
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FSMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,                /* 50 */
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,        /* 60 */
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,        /* 70 */
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    0,                              /* 79: CRYP, not present on STM32F407 */
    HASH_RNG_IRQHandler,            /* 80 */
    FPU_IRQHandler,
};

/* Weak so a lesson can supply its own clock setup. */
__attribute__((weak)) void SystemInit(void)
{
    /* Grant full access to coprocessors CP10/CP11 (the FPU); code built with
     * -mfloat-abi=hard faults on its first float instruction otherwise. */
    SCB->CPACR |= (3u << 20) | (3u << 22);
    __asm__ volatile("dsb\n isb");
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
