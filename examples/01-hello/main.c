/*
 * Lesson 1: the first program.
 *
 * Prints over the serial port and blinks the green LED. The UART and LED
 * helpers are black boxes for now; later lessons take them apart.
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/uart.h"

static void busy_delay(volatile uint32_t count)
{
    while (count-- != 0) {
    }
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);

    printf("Hello, STM32F407!\n");

    for (int i = 1; i <= 5; i++) {
        led_toggle(LED_GREEN);
        printf("blink %d, green LED is %s\n", i, (LED_PORT->ODR & BIT(LED_GREEN)) ? "on" : "off");
        busy_delay(100000);
    }

    printf("done\n");
    for (;;) {
    }
}
