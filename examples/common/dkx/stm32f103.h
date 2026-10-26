/*
 * Minimal STM32F103 register definitions for the LCSC "Dikuoxing"
 * STM32F103C8T6 board (appendix C).
 *
 * Same style as ../stm32f407.h: every peripheral is a struct that mirrors the
 * register map in the reference manual (RM0008), with compile-time offset
 * checks. Lesson 11 replaces this file with ST's official CMSIS header.
 */
#ifndef STM32F103_H
#define STM32F103_H

#include <stddef.h>
#include <stdint.h>

#define __IO volatile
#define __I  volatile const

#ifdef __cplusplus
#define STATIC_ASSERT static_assert
#else
#define STATIC_ASSERT _Static_assert
#endif

#define BIT(n) (1u << (n))

/* ------------------------------------------------------------------------- */
/* Memory map (RM0008 section 3.3)                                           */
/* ------------------------------------------------------------------------- */
#define FLASH_BASE      0x08000000u
#define SRAM_BASE       0x20000000u
#define PERIPH_BASE     0x40000000u

#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000u)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x10000u)
#define AHBPERIPH_BASE  (PERIPH_BASE + 0x20000u)

#define TIM2_BASE       (APB1PERIPH_BASE + 0x0000u)
#define TIM3_BASE       (APB1PERIPH_BASE + 0x0400u)
#define TIM4_BASE       (APB1PERIPH_BASE + 0x0800u)
#define USART2_BASE     (APB1PERIPH_BASE + 0x4400u)

#define AFIO_BASE       (APB2PERIPH_BASE + 0x0000u)
#define EXTI_BASE       (APB2PERIPH_BASE + 0x0400u)
#define GPIOA_BASE      (APB2PERIPH_BASE + 0x0800u)
#define GPIOB_BASE      (APB2PERIPH_BASE + 0x0C00u)
#define GPIOC_BASE      (APB2PERIPH_BASE + 0x1000u)
#define USART1_BASE     (APB2PERIPH_BASE + 0x3800u)

#define RCC_BASE        (AHBPERIPH_BASE + 0x1000u)
#define FLASH_R_BASE    (AHBPERIPH_BASE + 0x2000u)

#define SCS_BASE        0xE000E000u
#define SYSTICK_BASE    (SCS_BASE + 0x0010u)
#define NVIC_BASE       (SCS_BASE + 0x0100u)
#define SCB_BASE        (SCS_BASE + 0x0D00u)

/* ------------------------------------------------------------------------- */
/* RCC: reset and clock control                                              */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 clock control */
    __IO uint32_t CFGR;        /* 0x04 clock configuration (PLL settings live here too) */
    __IO uint32_t CIR;         /* 0x08 clock interrupt */
    __IO uint32_t APB2RSTR;    /* 0x0C */
    __IO uint32_t APB1RSTR;    /* 0x10 */
    __IO uint32_t AHBENR;      /* 0x14 AHB peripheral clock enable */
    __IO uint32_t APB2ENR;     /* 0x18 APB2 peripheral clock enable (GPIO, USART1, ...) */
    __IO uint32_t APB1ENR;     /* 0x1C APB1 peripheral clock enable */
    __IO uint32_t BDCR;        /* 0x20 backup domain control */
    __IO uint32_t CSR;         /* 0x24 control / status */
} RCC_TypeDef;

#define RCC_CR_HSION        BIT(0)
#define RCC_CR_HSIRDY       BIT(1)
#define RCC_CR_HSEON        BIT(16)
#define RCC_CR_HSERDY       BIT(17)
#define RCC_CR_PLLON        BIT(24)
#define RCC_CR_PLLRDY       BIT(25)

#define RCC_CFGR_SW_Pos       0u
#define RCC_CFGR_SW_Msk       (3u << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_PLL       (2u << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SWS_Pos      2u
#define RCC_CFGR_SWS_Msk      (3u << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL      (2u << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_HPRE_Pos     4u
#define RCC_CFGR_PPRE1_Pos    8u
#define RCC_CFGR_PPRE2_Pos    11u
#define RCC_CFGR_PLLSRC       BIT(16)    /* 0: HSI / 2, 1: HSE (after PLLXTPRE) */
#define RCC_CFGR_PLLXTPRE     BIT(17)    /* 1: HSE / 2 into the PLL */
#define RCC_CFGR_PLLMUL_Pos   18u        /* field value + 2 = multiplier, 2..16 */

#define RCC_APB2ENR_AFIOEN   BIT(0)
#define RCC_APB2ENR_IOPAEN   BIT(2)
#define RCC_APB2ENR_IOPBEN   BIT(3)
#define RCC_APB2ENR_IOPCEN   BIT(4)
#define RCC_APB2ENR_USART1EN BIT(14)

#define RCC_APB1ENR_TIM2EN   BIT(0)
#define RCC_APB1ENR_TIM3EN   BIT(1)
#define RCC_APB1ENR_TIM4EN   BIT(2)
#define RCC_APB1ENR_USART2EN BIT(17)

/* ------------------------------------------------------------------------- */
/* FLASH interface (only the access control register is needed)              */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t ACR;         /* 0x00 access control */
} FLASH_TypeDef;

#define FLASH_ACR_LATENCY_Msk 0x7u      /* 0: <= 24 MHz, 1: <= 48 MHz, 2: <= 72 MHz */
#define FLASH_ACR_PRFTBE      BIT(4)    /* prefetch buffer enable */

/* ------------------------------------------------------------------------- */
/* GPIO: each pin has a 4-bit field CNF[1:0] MODE[1:0] in CRL (pins 0-7) or  */
/* CRH (pins 8-15). There are no separate pull, speed or AF registers.       */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CRL;         /* 0x00 configuration, pins 0-7 */
    __IO uint32_t CRH;         /* 0x04 configuration, pins 8-15 */
    __I  uint32_t IDR;         /* 0x08 input data */
    __IO uint32_t ODR;         /* 0x0C output data (also selects pull-up/down in input mode) */
    __IO uint32_t BSRR;        /* 0x10 bit set (low half) / reset (high half) */
    __IO uint32_t BRR;         /* 0x14 bit reset */
    __IO uint32_t LCKR;        /* 0x18 configuration lock */
} GPIO_TypeDef;

/* The 4-bit value written to a pin's field: (CNF << 2) | MODE. */
enum {
    GPIO_CFG_ANALOG       = 0x0u,   /* input, CNF=00 MODE=00 */
    GPIO_CFG_IN_FLOATING  = 0x4u,   /* input, CNF=01: the reset state of every pin */
    GPIO_CFG_IN_PULL      = 0x8u,   /* input, CNF=10: ODR bit 1 = pull-up, 0 = pull-down */
    GPIO_CFG_OUT_PP_2MHZ  = 0x2u,   /* general purpose push-pull, MODE=10 (2 MHz) */
    GPIO_CFG_OUT_OD_2MHZ  = 0x6u,   /* general purpose open-drain */
    GPIO_CFG_AF_PP_50MHZ  = 0xBu,   /* alternate function push-pull, MODE=11 (50 MHz) */
    GPIO_CFG_AF_OD_50MHZ  = 0xFu,   /* alternate function open-drain */
};

enum {
    GPIO_PULL_DOWN = 0u,
    GPIO_PULL_UP   = 1u,
};

/* ------------------------------------------------------------------------- */
/* AFIO: pin remapping and the EXTI line -> port selection                   */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t EVCR;        /* 0x00 event control */
    __IO uint32_t MAPR;        /* 0x04 remap and debug configuration */
    __IO uint32_t EXTICR[4];   /* 0x08 EXTI line -> port selection */
} AFIO_TypeDef;

/* ------------------------------------------------------------------------- */
/* USART (same register layout as on the STM32F4)                            */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t SR;          /* 0x00 status */
    __IO uint32_t DR;          /* 0x04 data */
    __IO uint32_t BRR;         /* 0x08 baud rate */
    __IO uint32_t CR1;         /* 0x0C control 1 */
    __IO uint32_t CR2;         /* 0x10 control 2 */
    __IO uint32_t CR3;         /* 0x14 control 3 */
    __IO uint32_t GTPR;        /* 0x18 guard time and prescaler */
} USART_TypeDef;

#define USART_SR_ORE    BIT(3)
#define USART_SR_RXNE   BIT(5)
#define USART_SR_TC     BIT(6)
#define USART_SR_TXE    BIT(7)

#define USART_CR1_RE     BIT(2)
#define USART_CR1_TE     BIT(3)
#define USART_CR1_RXNEIE BIT(5)
#define USART_CR1_UE     BIT(13)

/* ------------------------------------------------------------------------- */
/* EXTI (same register layout as on the STM32F4)                             */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t IMR;         /* 0x00 interrupt mask */
    __IO uint32_t EMR;         /* 0x04 event mask */
    __IO uint32_t RTSR;        /* 0x08 rising trigger */
    __IO uint32_t FTSR;        /* 0x0C falling trigger */
    __IO uint32_t SWIER;       /* 0x10 software trigger */
    __IO uint32_t PR;          /* 0x14 pending (write 1 to clear) */
} EXTI_TypeDef;

/* ------------------------------------------------------------------------- */
/* General-purpose timers TIM2..TIM4 (same layout as on the STM32F4)         */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 control 1 */
    __IO uint32_t CR2;         /* 0x04 control 2 */
    __IO uint32_t SMCR;        /* 0x08 slave mode */
    __IO uint32_t DIER;        /* 0x0C DMA / interrupt enable */
    __IO uint32_t SR;          /* 0x10 status */
    __IO uint32_t EGR;         /* 0x14 event generation */
    __IO uint32_t CCMR1;       /* 0x18 capture/compare mode 1 */
    __IO uint32_t CCMR2;       /* 0x1C capture/compare mode 2 */
    __IO uint32_t CCER;        /* 0x20 capture/compare enable */
    __IO uint32_t CNT;         /* 0x24 counter */
    __IO uint32_t PSC;         /* 0x28 prescaler */
    __IO uint32_t ARR;         /* 0x2C auto-reload */
    uint32_t      RESERVED0;   /* 0x30 RCR exists only on TIM1 */
    __IO uint32_t CCR[4];      /* 0x34 capture/compare 1..4 */
} TIM_TypeDef;

#define TIM_CR1_CEN    BIT(0)
#define TIM_CR1_ARPE   BIT(7)
#define TIM_DIER_UIE   BIT(0)
#define TIM_SR_UIF     BIT(0)
#define TIM_EGR_UG     BIT(0)
#define TIM_CCER_CC3E  BIT(8)

#define TIM_CCMR2_OC3M_Pos 4u
#define TIM_CCMR2_OC3PE    BIT(3)
#define TIM_OCMODE_PWM1    6u

/* ------------------------------------------------------------------------- */
/* Cortex-M3 core peripherals                                                */
/* ------------------------------------------------------------------------- */
typedef struct {
    __IO uint32_t CTRL;        /* 0x00 control and status */
    __IO uint32_t LOAD;        /* 0x04 reload value */
    __IO uint32_t VAL;         /* 0x08 current value */
    __I  uint32_t CALIB;       /* 0x0C calibration */
} SysTick_TypeDef;

#define SYSTICK_CTRL_ENABLE    BIT(0)
#define SYSTICK_CTRL_TICKINT   BIT(1)
#define SYSTICK_CTRL_CLKSOURCE BIT(2)
#define SYSTICK_CTRL_COUNTFLAG BIT(16)

typedef struct {
    __IO uint32_t ISER[8];     /* 0x000 interrupt set-enable */
    uint32_t      RESERVED0[24];
    __IO uint32_t ICER[8];     /* 0x080 interrupt clear-enable */
    uint32_t      RESERVED1[24];
    __IO uint32_t ISPR[8];     /* 0x100 interrupt set-pending */
    uint32_t      RESERVED2[24];
    __IO uint32_t ICPR[8];     /* 0x180 interrupt clear-pending */
    uint32_t      RESERVED3[24];
    __IO uint32_t IABR[8];     /* 0x200 interrupt active */
    uint32_t      RESERVED4[56];
    __IO uint8_t  IP[240];     /* 0x300 interrupt priority, one byte per IRQ */
} NVIC_TypeDef;

typedef struct {
    __I  uint32_t CPUID;       /* 0x00 CPU identification */
    __IO uint32_t ICSR;        /* 0x04 interrupt control and state */
    __IO uint32_t VTOR;        /* 0x08 vector table offset */
    __IO uint32_t AIRCR;       /* 0x0C application interrupt and reset control */
    __IO uint32_t SCR;         /* 0x10 system control */
    __IO uint32_t CCR;         /* 0x14 configuration and control */
    __IO uint8_t  SHP[12];     /* 0x18 system handler priority */
    __IO uint32_t SHCSR;       /* 0x24 system handler control and state */
    __IO uint32_t CFSR;        /* 0x28 configurable fault status */
    __IO uint32_t HFSR;        /* 0x2C hard fault status */
    __IO uint32_t DFSR;        /* 0x30 */
    __IO uint32_t MMFAR;       /* 0x34 memory management fault address */
    __IO uint32_t BFAR;        /* 0x38 bus fault address */
    __IO uint32_t AFSR;        /* 0x3C */
} SCB_TypeDef;

#define SCB_CCR_DIV_0_TRP BIT(4)
#define SCB_SHCSR_MEMFAULTENA BIT(16)
#define SCB_SHCSR_BUSFAULTENA BIT(17)
#define SCB_SHCSR_USGFAULTENA BIT(18)

/* ------------------------------------------------------------------------- */
/* Peripheral instances                                                      */
/* ------------------------------------------------------------------------- */
#define RCC     ((RCC_TypeDef *)RCC_BASE)
#define FLASH   ((FLASH_TypeDef *)FLASH_R_BASE)
#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define AFIO    ((AFIO_TypeDef *)AFIO_BASE)
#define USART1  ((USART_TypeDef *)USART1_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)
#define EXTI    ((EXTI_TypeDef *)EXTI_BASE)
#define TIM2    ((TIM_TypeDef *)TIM2_BASE)
#define TIM3    ((TIM_TypeDef *)TIM3_BASE)
#define TIM4    ((TIM_TypeDef *)TIM4_BASE)
#define SYSTICK ((SysTick_TypeDef *)SYSTICK_BASE)
#define NVIC    ((NVIC_TypeDef *)NVIC_BASE)
#define SCB     ((SCB_TypeDef *)SCB_BASE)

/* ------------------------------------------------------------------------- */
/* Interrupt numbers (position in the vector table minus 16)                 */
/* ------------------------------------------------------------------------- */
typedef enum {
    EXTI0_IRQn  = 6,
    TIM2_IRQn   = 28,
    TIM3_IRQn   = 29,
    TIM4_IRQn   = 30,
    USART1_IRQn = 37,
    USART2_IRQn = 38,
} IRQn_Type;

static inline void nvic_enable_irq(IRQn_Type irq)
{
    NVIC->ISER[(uint32_t)irq >> 5] = 1u << ((uint32_t)irq & 31u);
}

static inline void nvic_disable_irq(IRQn_Type irq)
{
    NVIC->ICER[(uint32_t)irq >> 5] = 1u << ((uint32_t)irq & 31u);
}

/* STM32F1 implements the top 4 bits of each priority byte; 0 is the most urgent. */
static inline void nvic_set_priority(IRQn_Type irq, uint8_t priority)
{
    NVIC->IP[(uint32_t)irq] = (uint8_t)(priority << 4);
}

static inline void irq_disable(void) { __asm__ volatile("cpsid i" ::: "memory"); }
static inline void irq_enable(void)  { __asm__ volatile("cpsie i" ::: "memory"); }

static inline uint32_t irq_save(void)
{
    uint32_t primask;
    __asm__ volatile("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    return primask;
}

static inline void irq_restore(uint32_t primask)
{
    __asm__ volatile("msr primask, %0" :: "r"(primask) : "memory");
}

static inline void wait_for_interrupt(void) { __asm__ volatile("wfi"); }

/* ------------------------------------------------------------------------- */
/* Layout checks against RM0008 / ARMv7-M ARM                                */
/* ------------------------------------------------------------------------- */
STATIC_ASSERT(offsetof(RCC_TypeDef, APB2ENR) == 0x18, "RCC layout");
STATIC_ASSERT(offsetof(RCC_TypeDef, CSR) == 0x24, "RCC layout");
STATIC_ASSERT(offsetof(GPIO_TypeDef, BRR) == 0x14, "GPIO layout");
STATIC_ASSERT(offsetof(AFIO_TypeDef, EXTICR) == 0x08, "AFIO layout");
STATIC_ASSERT(offsetof(USART_TypeDef, GTPR) == 0x18, "USART layout");
STATIC_ASSERT(offsetof(EXTI_TypeDef, PR) == 0x14, "EXTI layout");
STATIC_ASSERT(offsetof(TIM_TypeDef, CCR) == 0x34, "TIM layout");
STATIC_ASSERT(offsetof(NVIC_TypeDef, ICER) == 0x80, "NVIC layout");
STATIC_ASSERT(offsetof(NVIC_TypeDef, IP) == 0x300, "NVIC layout");
STATIC_ASSERT(offsetof(SCB_TypeDef, CFSR) == 0x28, "SCB layout");

#endif
