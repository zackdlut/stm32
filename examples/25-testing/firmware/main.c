/*
 * Lesson 25 firmware: the tested modules wired to real hardware.
 * USART2 RX interrupt -> ring buffer -> line editor -> cmd_execute().
 */
#include <stdio.h>

#include "board.h"
#include "cmd.h"
#include "ringbuf.h"
#include "tick.h"
#include "uart.h"

static uint8_t g_rx_storage[64];
static ringbuf_t g_rx;
static volatile uint32_t g_rx_dropped;

void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_RXNE) {
        if (!rb_put(&g_rx, (uint8_t)USART2->DR)) {
            g_rx_dropped++;
        }
    }
}

static void board_led_set(int led, bool on)
{
    static const uint32_t pins[] = {LED_GREEN, LED_ORANGE, LED_RED, LED_BLUE};
    if (on) {
        led_on(pins[led]);
    } else {
        led_off(pins[led]);
    }
}

static const cmd_hw_t g_hw = {board_led_set, millis};

int main(void)
{
    char line[64];
    char reply[96];
    unsigned len = 0;

    board_init_leds();
    tick_init(HSI_HZ);
    rb_init(&g_rx, g_rx_storage, sizeof g_rx_storage);
    uart_init(HSI_HZ, 115200);
    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART2_IRQn);

    printf("\nlesson 25: command shell, type help\n> ");
    fflush(stdout);                                     /* stdout is line-buffered */
    for (;;) {
        uint8_t c;
        if (!rb_get(&g_rx, &c)) {
            __WFI();
            continue;
        }
        if (c == '\r' || c == '\n') {
            if (len == 0) {
                continue;
            }
            line[len] = '\0';
            len = 0;
            printf("\n");
            cmd_execute(line, reply, sizeof reply, &g_hw);
            printf("%s\n> ", reply);
            fflush(stdout);
        } else if (len < sizeof line - 1u) {
            line[len++] = (char)c;
            uart_putc((char)c);                         /* echo, like a terminal */
        }
    }
}
