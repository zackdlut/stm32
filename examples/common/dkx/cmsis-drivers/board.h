/*
 * LCSC "Dikuoxing" STM32F103C8T6 board helpers on top of ST's CMSIS header
 * (appendix C, lessons 16+).
 *
 *   PC13 user LED (high = on)
 *   PA0  external button to 3.3 V with a 10k pull-down (pressed = high)
 *   PA9  USART1 TX   PA10 USART1 RX
 */
#ifndef CMSIS_DRIVERS_BOARD_H
#define CMSIS_DRIVERS_BOARD_H

#include <stdint.h>

#include "stm32f1xx.h"

#define HSI_HZ 8000000u

/* CNF[1:0] MODE[1:0] nibble of CRL/CRH. */
enum {
    GPIO_CFG_ANALOG      = 0x0u,
    GPIO_CFG_IN_FLOATING = 0x4u,
    GPIO_CFG_IN_PULL     = 0x8u,   /* ODR bit picks up (1) or down (0) */
    GPIO_CFG_OUT_PP_2MHZ = 0x2u,
    GPIO_CFG_AF_PP_50MHZ = 0xBu,
    GPIO_CFG_AF_OD_50MHZ = 0xFu,
};

static inline void gpio_config(GPIO_TypeDef *port, uint32_t pin, uint32_t cfg)
{
    volatile uint32_t *cr = pin < 8u ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7u) * 4u;
    *cr = (*cr & ~(0xFu << shift)) | (cfg << shift);
}

static inline void gpio_set_pull_up(GPIO_TypeDef *port, uint32_t pin)
{
    port->BSRR = 1u << pin;
    gpio_config(port, pin, GPIO_CFG_IN_PULL);
}

#define LED_PIN 13u

static inline void board_init_led(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    gpio_config(GPIOC, LED_PIN, GPIO_CFG_OUT_PP_2MHZ);
}

static inline void led_on(void)  { GPIOC->BSRR = 1u << LED_PIN; }
static inline void led_off(void) { GPIOC->BRR = 1u << LED_PIN; }
static inline void led_toggle(void)
{
    GPIOC->BSRR = (GPIOC->ODR & (1u << LED_PIN)) ? 1u << (LED_PIN + 16u) : 1u << LED_PIN;
}

#endif
