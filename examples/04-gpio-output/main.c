/*
 * Lesson 4: GPIO output.
 *
 * Step 1 lights the green LED with nothing but raw addresses, so every
 * register access is visible. Step 2 runs a four-LED chaser using the struct
 * definitions and the atomic BSRR register.
 */
#include <stdio.h>

#include "drivers/uart.h"
#include "stm32f407.h"

/* ---- Step 1: raw registers, straight from the reference manual ---------- */
#define REG32(addr)      (*(volatile uint32_t *)(addr))
#define RCC_AHB1ENR_RAW  REG32(0x40023800u + 0x30u)
#define GPIOD_MODER_RAW  REG32(0x40020C00u + 0x00u)
#define GPIOD_ODR_RAW_   REG32(0x40020C00u + 0x14u)

static void step1_raw_green_led(void)
{
    RCC_AHB1ENR_RAW |= (1u << 3);                 /* 1. clock GPIOD (bit 3 = GPIODEN) */
    GPIOD_MODER_RAW &= ~(3u << (12 * 2));         /* 2. clear PD12 mode bits ...      */
    GPIOD_MODER_RAW |=  (1u << (12 * 2));         /*    ... and select 01 = output    */
    GPIOD_ODR_RAW_  |=  (1u << 12);               /* 3. drive PD12 high: LED on       */
}

/* ---- Step 2: struct access and BSRR ------------------------------------- */
static void delay(volatile uint32_t count)
{
    while (count-- != 0) {
    }
}

static void leds_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    for (uint32_t pin = 12; pin <= 15; pin++) {
        GPIOD->MODER   = (GPIOD->MODER & ~(3u << (pin * 2))) | (GPIO_MODE_OUTPUT << (pin * 2));
        GPIOD->OTYPER &= ~BIT(pin);                      /* push-pull */
        GPIOD->OSPEEDR &= ~(3u << (pin * 2));            /* low speed is plenty for an LED */
        GPIOD->PUPDR  &= ~(3u << (pin * 2));             /* no pull resistor */
    }
}

static void show_one(uint32_t pin)
{
    /* Low half of BSRR sets pins, high half resets them; one write does both. */
    uint32_t set = BIT(pin);
    uint32_t reset = (0xFu << 12) & ~set;
    GPIOD->BSRR = set | (reset << 16);
}

int main(void)
{
    uart_init(16000000u, 115200);

    step1_raw_green_led();
    printf("step 1: GPIOD MODER=0x%08lx ODR=0x%04lx -> green LED on\n",
           (unsigned long)GPIOD->MODER, (unsigned long)GPIOD->ODR);
    delay(50000);

    leds_init();
    static const char *const names[] = {"green", "orange", "red", "blue"};

    printf("step 2: chaser\n");
    for (int round = 0; round < 2; round++) {
        for (uint32_t pin = 12; pin <= 15; pin++) {
            show_one(pin);
            printf("  ODR=0x%04lx  %s\n", (unsigned long)GPIOD->ODR, names[pin - 12]);
            delay(50000);
        }
    }

    GPIOD->BSRR = (0xFu << 12) << 16;
    printf("all LEDs off, ODR=0x%04lx\n", (unsigned long)GPIOD->ODR);

    for (;;) {
    }
}
