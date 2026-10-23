/*
 * Lesson 5: GPIO input, polling and debouncing.
 *
 * The button on PA0 is sampled roughly once per millisecond. Two counters run
 * side by side:
 *   raw_edges     counts every low->high transition seen on the pin
 *   debounced     accepts a new level only after it has been stable for
 *                 DEBOUNCE_SAMPLES consecutive samples
 * A bouncing contact inflates raw_edges but not debounced presses.
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/uart.h"

#define DEBOUNCE_SAMPLES 20u

/* Roughly 1 ms at 16 MHz; lesson 6 replaces this guess with a real timer. */
static void delay_about_1ms(void)
{
    for (volatile uint32_t i = 0; i < 1600u; i++) {
    }
}

int main(void)
{
    board_init_leds();
    board_init_button();
    uart_init(HSI_HZ, 115200);

    printf("PA0 MODER=%lu PUPDR=%lu (0=input, 2=pull-down)\n",
           (unsigned long)(GPIOA->MODER & 3u), (unsigned long)(GPIOA->PUPDR & 3u));
    printf("waiting for button presses...\n");

    int last_raw = 0;
    int stable_level = 0;
    uint32_t stable_count = 0;
    uint32_t raw_edges = 0;
    uint32_t presses = 0;

    for (;;) {
        int raw = button_pressed();

        if (raw && !last_raw) {
            raw_edges++;
        }
        last_raw = raw;

        if (raw == stable_level) {
            stable_count = 0;
        } else if (++stable_count >= DEBOUNCE_SAMPLES) {
            stable_level = raw;
            stable_count = 0;
            if (stable_level) {
                presses++;
                led_toggle(LED_ORANGE);
                printf("press #%lu  (raw rising edges so far: %lu)\n",
                       (unsigned long)presses, (unsigned long)raw_edges);
            } else {
                printf("release\n");
            }
        }

        delay_about_1ms();
    }
}
