/*
 * Lesson 1, Dikuoxing STM32F103C8T6 version: the first program.
 *
 * Prints over USART1 (PA9/PA10) and blinks the PC13 user LED. The UART and
 * LED helpers are black boxes for now; later lessons take them apart.
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
    board_init_led();
    uart_init(HSI_HZ, 115200);

    printf("Hello, STM32F103!\n");

    for (int i = 1; i <= 5; i++) {
        led_toggle();
        printf("blink %d, PC13 LED is %s\n", i, led_is_on() ? "on" : "off");
        busy_delay(50000);
    }

    printf("done\n");
    for (;;) {
    }
}
