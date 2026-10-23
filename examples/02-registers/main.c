/*
 * Lesson 2: peripherals are memory.
 *
 * The same GPIOD output register is reached three ways: a raw address, a
 * pointer macro and a struct overlay. Then the four basic bit operations
 * (set, clear, toggle, test) and a multi-bit field update.
 */
#include <stdio.h>

#include "drivers/uart.h"
#include "stm32f407.h"

/* RM0090: GPIOD starts at 0x4002_0C00, ODR is at offset 0x14. */
#define GPIOD_ODR_ADDR  0x40020C14u
#define GPIOD_ODR_RAW   (*(volatile uint32_t *)GPIOD_ODR_ADDR)

/*
 * Two ways to wait for "transmit register empty". Compare their machine code
 * with `make disasm`: without volatile the compiler reads SR once and may spin
 * forever on a stale copy.
 */
__attribute__((noinline)) void wait_txe_without_volatile(void)
{
    uint32_t *sr = (uint32_t *)&USART2->SR;
    while ((*sr & USART_SR_TXE) == 0) {
    }
}

__attribute__((noinline)) void wait_txe_with_volatile(void)
{
    while ((USART2->SR & USART_SR_TXE) == 0) {
    }
}

static void show(const char *what)
{
    printf("%-28s MODER=0x%08lx ODR=0x%04lx\n", what,
           (unsigned long)GPIOD->MODER, (unsigned long)GPIOD->ODR);
}

int main(void)
{
    uart_init(16000000u, 115200);

    printf("\n== Same register, three spellings ==\n");
    printf("raw address      : 0x%08lx\n", (unsigned long)GPIOD_ODR_ADDR);
    printf("&GPIOD_ODR_RAW   : 0x%08lx\n", (unsigned long)&GPIOD_ODR_RAW);
    printf("&GPIOD->ODR      : 0x%08lx\n", (unsigned long)&GPIOD->ODR);

    printf("\n== Read-only identification register ==\n");
    printf("SCB->CPUID       : 0x%08lx (0x410FC24x = Cortex-M4, x = revision)\n", (unsigned long)SCB->CPUID);

    printf("\n== Bit operations on GPIOD ==\n");
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    show("after reset:");

    /* Field update: MODER has 2 bits per pin; set PD12..PD15 to 01 (output). */
    uint32_t moder = GPIOD->MODER;
    moder &= ~(0xFFu << 24);
    moder |= (0x55u << 24);
    GPIOD->MODER = moder;
    show("PD12-15 -> output:");

    GPIOD_ODR_RAW |= BIT(12);
    show("set bit 12 (raw):");

    GPIOD->ODR |= BIT(13) | BIT(14);
    show("set bits 13,14:");

    GPIOD->ODR &= ~BIT(13);
    show("clear bit 13:");

    GPIOD->ODR ^= BIT(15);
    show("toggle bit 15:");

    GPIOD->BSRR = BIT(12 + 16);
    show("BSRR reset bit 12:");

    printf("test bit 14      : %s\n", (GPIOD->ODR & BIT(14)) ? "set" : "clear");

    printf("\n== volatile ==\n");
    printf("wait_txe_without_volatile @ 0x%08lx\n", (unsigned long)wait_txe_without_volatile);
    printf("wait_txe_with_volatile    @ 0x%08lx\n", (unsigned long)wait_txe_with_volatile);
    wait_txe_with_volatile();
    printf("run `make disasm` and compare them in build/firmware.lst\n");

    for (;;) {
    }
}
