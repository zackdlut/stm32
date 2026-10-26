/*
 * Lesson 2, Dikuoxing STM32F103C8T6 version: peripherals are memory.
 *
 * The same GPIOC output register is reached three ways: a raw address, a
 * pointer macro and a struct overlay. Then the four basic bit operations
 * (set, clear, toggle, test) and a multi-bit field update. On the F103 that
 * field is the 4-bit CNF/MODE nibble of PC13 in GPIOC_CRH.
 */
#include <stdio.h>

#include "drivers/uart.h"
#include "stm32f103.h"

/* RM0008: GPIOC starts at 0x4001_1000, ODR is at offset 0x0C. */
#define GPIOC_ODR_ADDR  0x4001100Cu
#define GPIOC_ODR_RAW   (*(volatile uint32_t *)GPIOC_ODR_ADDR)

/*
 * Two ways to wait for "transmit register empty". Compare their machine code
 * with `make disasm BOARD=dkx`: without volatile the compiler reads SR once
 * and may spin forever on a stale copy.
 */
__attribute__((noinline)) void wait_txe_without_volatile(void)
{
    uint32_t *sr = (uint32_t *)&USART1->SR;
    while ((*sr & USART_SR_TXE) == 0) {
    }
}

__attribute__((noinline)) void wait_txe_with_volatile(void)
{
    while ((USART1->SR & USART_SR_TXE) == 0) {
    }
}

static void show(const char *what)
{
    printf("%-28s CRH=0x%08lx ODR=0x%04lx\n", what,
           (unsigned long)GPIOC->CRH, (unsigned long)GPIOC->ODR);
}

int main(void)
{
    uart_init(8000000u, 115200);

    printf("\n== Same register, three spellings ==\n");
    printf("raw address      : 0x%08lx\n", (unsigned long)GPIOC_ODR_ADDR);
    printf("&GPIOC_ODR_RAW   : 0x%08lx\n", (unsigned long)&GPIOC_ODR_RAW);
    printf("&GPIOC->ODR      : 0x%08lx\n", (unsigned long)&GPIOC->ODR);

    printf("\n== Read-only identification register ==\n");
    printf("SCB->CPUID       : 0x%08lx (0x41xFC23x = Cortex-M3, x = revision)\n", (unsigned long)SCB->CPUID);

    printf("\n== Bit operations on GPIOC ==\n");
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    show("after reset:");

    /* Field update: CRH has 4 bits per pin (pins 8-15). PC13 is the sixth
     * nibble, bits 23:20. 0010 = general purpose push-pull output, 2 MHz. */
    uint32_t crh = GPIOC->CRH;
    crh &= ~(0xFu << 20);
    crh |= (0x2u << 20);
    GPIOC->CRH = crh;
    show("PC13 -> output:");

    GPIOC_ODR_RAW |= BIT(13);
    show("set bit 13 (raw):");

    GPIOC->ODR |= BIT(14) | BIT(15);
    show("set bits 14,15:");

    GPIOC->ODR &= ~BIT(14);
    show("clear bit 14:");

    GPIOC->ODR ^= BIT(15);
    show("toggle bit 15:");

    GPIOC->BSRR = BIT(13 + 16);
    show("BSRR reset bit 13:");

    GPIOC->BSRR = BIT(13);
    GPIOC->BRR = BIT(13);
    show("BSRR set, BRR reset bit 13:");

    printf("test bit 13      : %s\n", (GPIOC->ODR & BIT(13)) ? "set" : "clear");

    printf("\n== volatile ==\n");
    printf("wait_txe_without_volatile @ 0x%08lx\n", (unsigned long)wait_txe_without_volatile);
    printf("wait_txe_with_volatile    @ 0x%08lx\n", (unsigned long)wait_txe_with_volatile);
    wait_txe_with_volatile();
    printf("run `make disasm BOARD=dkx` and compare them in build-dkx/firmware.lst\n");

    for (;;) {
    }
}
