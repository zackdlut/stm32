/*
 * LCSC "Dikuoxing" STM32F103C8T6 board wiring.
 *
 *   PC13 user LED (high = on)
 *   PA0  external button to 3.3 V with a 10k pull-down (pressed = high)
 *   PA9  USART1 TX   PA10 USART1 RX   (to a USB-TTL adapter)
 *
 * PC13 can only source about 3 mA and must not switch faster than 2 MHz
 * (RM0008, "GPIO" and the datasheet's PC13-PC15 note), fine for an LED.
 */
#ifndef DRIVERS_BOARD_H
#define DRIVERS_BOARD_H

#include "stm32f103.h"
#include "drivers/gpio.h"

/* After reset the core runs from the internal 8 MHz HSI RC oscillator. */
#define HSI_HZ 8000000u

#define LED_PORT GPIOC
#define LED_PIN  13u

#define BUTTON_PORT GPIOA
#define BUTTON_PIN  0u

static inline void board_init_led(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    gpio_config(LED_PORT, LED_PIN, GPIO_CFG_OUT_PP_2MHZ);
}

static inline void board_init_button(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    gpio_set_pull(BUTTON_PORT, BUTTON_PIN, GPIO_PULL_DOWN);
}

static inline void led_on(void)     { gpio_write(LED_PORT, LED_PIN, 1); }
static inline void led_off(void)    { gpio_write(LED_PORT, LED_PIN, 0); }
static inline void led_toggle(void) { gpio_toggle(LED_PORT, LED_PIN); }
static inline int led_is_on(void)   { return (int)((LED_PORT->ODR >> LED_PIN) & 1u); }

static inline int button_pressed(void)
{
    return gpio_read(BUTTON_PORT, BUTTON_PIN);
}

#endif
