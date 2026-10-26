/*
 * Lesson 9, Dikuoxing STM32F103C8T6 version: interrupt-driven UART reception
 * and a tiny command line.
 *
 *   USART1 RXNE --> USART1_IRQHandler --> ring buffer --> main loop --> parser
 *
 * The handler does the minimum (move one byte into the buffer); parsing and
 * printing happen in main(), outside interrupt context. The board has a single
 * user LED (PC13), so the led command takes no LED name.
 */
#include <stdio.h>
#include <string.h>

#include "drivers/board.h"
#include "drivers/systick.h"
#include "drivers/uart.h"
#include "ringbuf.h"

static ringbuf_t g_rx;
static volatile uint32_t g_rx_dropped;
static volatile uint32_t g_rx_overruns;
static uint32_t g_rx_peak;

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        /* Reading DR clears RXNE, and (after the SR read above) ORE too. */
        uint8_t byte = (uint8_t)USART1->DR;
        if (sr & USART_SR_ORE) {
            g_rx_overruns++;
        }
        if (!ringbuf_put(&g_rx, byte)) {
            g_rx_dropped++;
        }
    }
}

static void uart_rx_irq_init(void)
{
    USART1->CR1 |= USART_CR1_RXNEIE;
    nvic_set_priority(USART1_IRQn, 6);
    nvic_enable_irq(USART1_IRQn);
}

/* ---- Commands ------------------------------------------------------------ */
static void cmd_help(void)
{
    printf("commands:\n"
           "  help                  this text\n"
           "  led <on|off|toggle>   PC13 user LED\n"
           "  status                LED state and UART statistics\n"
           "  uptime                milliseconds since boot\n");
}

static void cmd_led(char *args)
{
    char *action = strtok(args, " ");

    if (action == NULL) {
        printf("usage: led <on|off|toggle>\n");
        return;
    }
    if (strcmp(action, "on") == 0) {
        led_on();
    } else if (strcmp(action, "off") == 0) {
        led_off();
    } else if (strcmp(action, "toggle") == 0) {
        led_toggle();
    } else {
        printf("unknown action '%s'\n", action);
        return;
    }
    printf("led is %s\n", led_is_on() ? "on" : "off");
}

static void cmd_status(void)
{
    printf("LED: %s\n", led_is_on() ? "on" : "off");
    printf("RX buffer: peak %lu/%u bytes, dropped %lu, overruns %lu\n",
           (unsigned long)g_rx_peak, (unsigned)RINGBUF_SIZE,
           (unsigned long)g_rx_dropped, (unsigned long)g_rx_overruns);
}

static void execute(char *line)
{
    char *cmd = strtok(line, " ");
    char *rest = strtok(NULL, "");

    if (cmd == NULL) {
        return;
    } else if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "led") == 0) {
        cmd_led(rest ? rest : "");
    } else if (strcmp(cmd, "status") == 0) {
        cmd_status();
    } else if (strcmp(cmd, "uptime") == 0) {
        printf("%lu ms\n", (unsigned long)millis());
    } else {
        printf("unknown command '%s', try 'help'\n", cmd);
    }
}

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    systick_init(HSI_HZ);
    uart_rx_irq_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\nSTM32F103 CLI. Type 'help'.\n> ");

    char line[64];
    size_t len = 0;

    for (;;) {
        uint8_t c;
        uint32_t level = ringbuf_count(&g_rx);
        if (level > g_rx_peak) {
            g_rx_peak = level;
        }

        if (!ringbuf_get(&g_rx, &c)) {
            wait_for_interrupt();          /* nothing to do until the next byte */
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (len == 0) {
                continue;
            }
            line[len] = '\0';
            printf("\n");
            execute(line);
            len = 0;
            printf("> ");
        } else if ((c == '\b' || c == 0x7F) && len > 0) {
            len--;
            printf("\b \b");
        } else if (c >= ' ' && len + 1 < sizeof line) {
            line[len++] = (char)c;
            uart_putc((char)c);            /* echo */
        }
    }
}
