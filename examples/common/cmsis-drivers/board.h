/*
 * STM32F4 Discovery board helpers on top of ST's CMSIS header (lessons 16+).
 *
 *   PD12 green LED   PD13 orange LED   PD14 red LED   PD15 blue LED
 *   PA0  user button (pressed = high)
 *   PA2  USART2 TX   PA3  USART2 RX
 */
#ifndef CMSIS_DRIVERS_BOARD_H
#define CMSIS_DRIVERS_BOARD_H

#include <stdint.h>

#include "stm32f4xx.h"

#define HSI_HZ 16000000u

enum {
    LED_GREEN  = 12,
    LED_ORANGE = 13,
    LED_RED    = 14,
    LED_BLUE   = 15,
};

enum {
    GPIO_MODE_INPUT  = 0u,
    GPIO_MODE_OUTPUT = 1u,
    GPIO_MODE_AF     = 2u,
    GPIO_MODE_ANALOG = 3u,
};

enum {
    GPIO_PULL_NONE = 0u,
    GPIO_PULL_UP   = 1u,
    GPIO_PULL_DOWN = 2u,
};

static inline void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    port->MODER = (port->MODER & ~(3u << (pin * 2u))) | (mode << (pin * 2u));
}

static inline void gpio_set_pull(GPIO_TypeDef *port, uint32_t pin, uint32_t pull)
{
    port->PUPDR = (port->PUPDR & ~(3u << (pin * 2u))) | (pull << (pin * 2u));
}

static inline void gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    uint32_t shift = (pin & 7u) * 4u;
    port->AFR[pin >> 3] = (port->AFR[pin >> 3] & ~(0xFu << shift)) | (af << shift);
    gpio_set_mode(port, pin, GPIO_MODE_AF);
}

static inline void board_init_leds(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    for (uint32_t pin = LED_GREEN; pin <= LED_BLUE; pin++) {
        gpio_set_mode(GPIOD, pin, GPIO_MODE_OUTPUT);
    }
}

static inline void led_on(uint32_t led)  { GPIOD->BSRR = 1u << led; }
static inline void led_off(uint32_t led) { GPIOD->BSRR = 1u << (led + 16u); }
static inline void led_toggle(uint32_t led)
{
    GPIOD->BSRR = (GPIOD->ODR & (1u << led)) ? 1u << (led + 16u) : 1u << led;
}

#endif
