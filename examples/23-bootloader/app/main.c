/*
 * Lesson 23: the application started by the bootloader.
 * Built twice, with APP_VERSION=1 and APP_VERSION=2.
 */
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

extern const uint32_t g_pfnVectors[];       /* ST startup file's vector table */

int main(void)
{
    const uint32_t led = APP_VERSION == 1 ? LED_GREEN : LED_BLUE;

    board_init_leds();
    uart_init(HSI_HZ, 115200);
    printf("app v%d: running, VTOR=0x%08lx, my vector table at 0x%08lx\n", APP_VERSION,
           (unsigned long)SCB->VTOR, (unsigned long)(uintptr_t)g_pfnVectors);

    tick_init(HSI_HZ);                      /* SysTick_Handler is found through VTOR */
    for (int i = 0; i < 3; i++) {
        led_toggle(led);
        delay_ms(50);
    }
    printf("app v%d: SysTick interrupts work, t=%lu ms\n", APP_VERSION,
           (unsigned long)millis());

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    gpio_set_mode(GPIOA, 0, GPIO_MODE_INPUT);
    for (;;) {
        if (GPIOA->IDR & 1u) {
            printf("app v%d: button pressed, resetting\n", APP_VERSION);
            uart_flush();
            NVIC_SystemReset();
        }
        delay_ms(10);
    }
}
