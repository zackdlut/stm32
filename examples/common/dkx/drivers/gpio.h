/*
 * GPIO helpers for the STM32F103 (lessons 4 and 5, Dikuoxing version).
 *
 * Unlike the F4, a pin's whole configuration is one 4-bit field in CRL
 * (pins 0-7) or CRH (pins 8-15). Pull-up versus pull-down is not a separate
 * register: in GPIO_CFG_IN_PULL mode the pin's ODR bit picks the direction.
 * Alternate functions are fixed per pin (remapping is done in AFIO->MAPR).
 */
#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

#include "stm32f103.h"

static inline void gpio_config(GPIO_TypeDef *port, uint32_t pin, uint32_t cfg)
{
    volatile uint32_t *cr = (pin < 8u) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7u) * 4u;
    *cr = (*cr & ~(0xFu << shift)) | (cfg << shift);
}

static inline void gpio_set_pull(GPIO_TypeDef *port, uint32_t pin, uint32_t pull)
{
    port->BSRR = pull == GPIO_PULL_UP ? BIT(pin) : BIT(pin + 16u);
    gpio_config(port, pin, GPIO_CFG_IN_PULL);
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
