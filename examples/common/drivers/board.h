/*
 * STM32F4 Discovery board wiring.
 *
 *   PD12 green LED   PD13 orange LED   PD14 red LED   PD15 blue LED
 *   PA0  user button (pressed = high, external pull-down on the board)
 *   PA2  USART2 TX   PA3  USART2 RX   (debug serial port)
 */
#ifndef DRIVERS_BOARD_H
#define DRIVERS_BOARD_H

#include "stm32f407.h"
#include "drivers/gpio.h"

/* After reset the core runs from the internal 16 MHz HSI RC oscillator. */
#define HSI_HZ 16000000u

#define LED_PORT GPIOD
enum {
    LED_GREEN  = 12,
    LED_ORANGE = 13,
    LED_RED    = 14,
    LED_BLUE   = 15,
};

#define BUTTON_PORT GPIOA
#define BUTTON_PIN  0u

static inline void board_init_leds(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    for (uint32_t pin = LED_GREEN; pin <= LED_BLUE; pin++) {
        gpio_set_mode(LED_PORT, pin, GPIO_MODE_OUTPUT);
    }
}

static inline void board_init_button(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    gpio_set_mode(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull(BUTTON_PORT, BUTTON_PIN, GPIO_PULL_DOWN);
}

static inline void led_on(uint32_t led)     { gpio_write(LED_PORT, led, 1); }
static inline void led_off(uint32_t led)    { gpio_write(LED_PORT, led, 0); }
static inline void led_toggle(uint32_t led) { gpio_toggle(LED_PORT, led); }

static inline int button_pressed(void)
{
    return gpio_read(BUTTON_PORT, BUTTON_PIN);
}

#endif
