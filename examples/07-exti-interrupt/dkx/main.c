/*
 * Lesson 7, Dikuoxing STM32F103C8T6 version: interrupts, NVIC and EXTI.
 *
 * The button on PA0 raises EXTI line 0 on its rising edge. The interrupt
 * handler records the event; main() sleeps with WFI and prints whatever the
 * handler queued. A 50 ms dead time in the handler rejects contact bounce.
 *
 *   PA0 --> AFIO_EXTICR1 (port A) --> EXTI line 0 --> NVIC IRQ 6 --> EXTI0_IRQHandler
 *
 * The only difference from the F4 is the multiplexer in front of EXTI: on the
 * F1 it lives in AFIO (clock bit AFIOEN in APB2ENR) instead of SYSCFG.
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/systick.h"
#include "drivers/uart.h"

#define DEAD_TIME_MS 50u

/* Shared between the interrupt handler and main(): must be volatile. */
static volatile uint32_t g_raw_edges;
static volatile uint32_t g_presses;
static volatile uint32_t g_last_press_ms;
static volatile uint8_t g_event_pending;

static void button_exti_init(void)
{
    board_init_button();

    /* 1. Route PA0 to EXTI line 0. EXTICR1 bits [3:0] select the port: 0 = A. */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~0xFu) | 0x0u;

    /* 2. EXTI: unmask line 0, trigger on the rising edge only. */
    EXTI->IMR  |= BIT(0);
    EXTI->RTSR |= BIT(0);
    EXTI->FTSR &= ~BIT(0);
    EXTI->PR    = BIT(0);                 /* clear any stale pending flag */

    /* 3. NVIC: priority then enable. Lower number = more urgent. */
    nvic_set_priority(EXTI0_IRQn, 5);
    nvic_enable_irq(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void)
{
    /* Clear first. PR is write-1-to-clear; forgetting this re-enters forever. */
    EXTI->PR = BIT(0);

    g_raw_edges++;

    uint32_t now = millis();
    if (g_presses != 0 && (uint32_t)(now - g_last_press_ms) < DEAD_TIME_MS) {
        return;                            /* bounce: ignore */
    }

    g_last_press_ms = now;
    g_presses++;
    led_toggle();
    g_event_pending = 1;
}

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    systick_init(HSI_HZ);
    button_exti_init();

    printf("EXTI0 ready: IMR=0x%lx RTSR=0x%lx, NVIC ISER0=0x%08lx\n",
           (unsigned long)EXTI->IMR, (unsigned long)EXTI->RTSR, (unsigned long)NVIC->ISER[0]);
    printf("main() sleeps in WFI; the button wakes it up\n");

    for (;;) {
        /* Snapshot shared state with interrupts masked so the values match. */
        uint32_t primask = irq_save();
        uint8_t pending = g_event_pending;
        uint32_t presses = g_presses;
        uint32_t edges = g_raw_edges;
        uint32_t when = g_last_press_ms;
        g_event_pending = 0;
        irq_restore(primask);

        if (pending) {
            printf("t=%4lu ms  press #%lu  (interrupts so far: %lu)\n",
                   (unsigned long)when, (unsigned long)presses, (unsigned long)edges);
        }

        wait_for_interrupt();
    }
}
