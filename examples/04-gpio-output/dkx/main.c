/*
 * Lesson 4, Dikuoxing STM32F103C8T6 version: GPIO output.
 *
 * Step 1 lights the PC13 LED with nothing but raw addresses, so every
 * register access is visible (the same three steps as the LCKFB "register
 * LED" chapter). Step 2 blinks it through the structs, with the atomic
 * BSRR/BRR registers instead of read-modify-write on ODR.
 *
 * The board has a single user LED. For a chaser like the F407 version, wire
 * LEDs (with ~1k resistors to GND) to PA1..PA3 and add them to `pins` below.
 */
#include <stdio.h>

#include "drivers/uart.h"
#include "stm32f103.h"

/* ---- Step 1: raw registers, straight from the reference manual ---------- */
#define REG32(addr)      (*(volatile uint32_t *)(addr))
#define RCC_APB2ENR_RAW  REG32(0x40021000u + 0x18u)
#define GPIOC_CRH_RAW    REG32(0x40011000u + 0x04u)
#define GPIOC_ODR_RAW_   REG32(0x40011000u + 0x0Cu)

static void step1_raw_led(void)
{
    RCC_APB2ENR_RAW |= (1u << 4);                 /* 1. clock GPIOC (bit 4 = IOPCEN)      */
    GPIOC_CRH_RAW &= ~(0xFu << ((13 - 8) * 4));   /* 2. clear PC13's CNF/MODE nibble ...  */
    GPIOC_CRH_RAW |=  (0x2u << ((13 - 8) * 4));   /*    ... 0010 = push-pull output 2 MHz */
    GPIOC_ODR_RAW_ |= (1u << 13);                 /* 3. drive PC13 high: LED on           */
}

/* ---- Step 2: struct access, BSRR and BRR -------------------------------- */
static void delay(volatile uint32_t count)
{
    while (count-- != 0) {
    }
}

static void pin_output(GPIO_TypeDef *port, uint32_t pin)
{
    volatile uint32_t *cr = (pin < 8u) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7u) * 4u;
    *cr = (*cr & ~(0xFu << shift)) | (GPIO_CFG_OUT_PP_2MHZ << shift);
}

int main(void)
{
    uart_init(8000000u, 115200);

    step1_raw_led();
    printf("step 1: GPIOC CRH=0x%08lx ODR=0x%04lx -> PC13 LED on\n",
           (unsigned long)GPIOC->CRH, (unsigned long)GPIOC->ODR);
    delay(25000);

    static const struct {
        GPIO_TypeDef *port;
        uint32_t pin;
        const char *name;
    } pins[] = {
        {GPIOC, 13, "PC13"},
    };
    const unsigned count = sizeof pins / sizeof pins[0];

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;
    for (unsigned i = 0; i < count; i++) {
        pin_output(pins[i].port, pins[i].pin);
    }

    printf("step 2: blink with BSRR (set) and BRR (reset)\n");
    for (int round = 0; round < 3; round++) {
        for (unsigned i = 0; i < count; i++) {
            pins[i].port->BSRR = BIT(pins[i].pin);
            printf("  ODR=0x%04lx  %s on\n", (unsigned long)pins[i].port->ODR, pins[i].name);
            delay(25000);
            pins[i].port->BRR = BIT(pins[i].pin);
            printf("  ODR=0x%04lx  %s off\n", (unsigned long)pins[i].port->ODR, pins[i].name);
            delay(25000);
        }
    }

    printf("done, GPIOC ODR=0x%04lx\n", (unsigned long)GPIOC->ODR);

    for (;;) {
    }
}
