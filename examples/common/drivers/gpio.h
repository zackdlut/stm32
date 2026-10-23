/*
 * GPIO helpers distilled from lessons 4 and 5. Each pin owns a 2-bit field
 * in MODER/OSPEEDR/PUPDR, a 1-bit field in OTYPER/IDR/ODR and a 4-bit field
 * in AFR[0] (pins 0-7) or AFR[1] (pins 8-15).
 */
#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

#include "stm32f407.h"

static inline void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    uint32_t shift = pin * 2u;
    port->MODER = (port->MODER & ~(3u << shift)) | (mode << shift);
}

static inline void gpio_set_pull(GPIO_TypeDef *port, uint32_t pin, uint32_t pull)
{
    uint32_t shift = pin * 2u;
    port->PUPDR = (port->PUPDR & ~(3u << shift)) | (pull << shift);
}

static inline void gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    uint32_t idx = pin >> 3;
    uint32_t shift = (pin & 7u) * 4u;
    port->AFR[idx] = (port->AFR[idx] & ~(0xFu << shift)) | (af << shift);
    gpio_set_mode(port, pin, GPIO_MODE_AF);
}

/* BSRR writes are atomic: no read-modify-write, so an interrupt cannot corrupt them. */
static inline void gpio_write(GPIO_TypeDef *port, uint32_t pin, int high)
{
    port->BSRR = high ? BIT(pin) : BIT(pin + 16u);
}

static inline void gpio_toggle(GPIO_TypeDef *port, uint32_t pin)
{
    gpio_write(port, pin, !((port->ODR >> pin) & 1u));
}

static inline int gpio_read(GPIO_TypeDef *port, uint32_t pin)
{
    return (int)((port->IDR >> pin) & 1u);
}

#endif
