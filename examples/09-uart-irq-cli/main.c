/*
 * Lesson 9: interrupt-driven UART reception and a tiny command line.
 *
 *   USART2 RXNE --> USART2_IRQHandler --> ring buffer --> main loop --> parser
 *
 * The handler does the minimum (move one byte into the buffer); parsing and
 * printing happen in main(), outside interrupt context.
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

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;

    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        /* Reading DR clears RXNE, and (after the SR read above) ORE too. */
        uint8_t byte = (uint8_t)USART2->DR;
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
    USART2->CR1 |= USART_CR1_RXNEIE;
    nvic_set_priority(USART2_IRQn, 6);
    nvic_enable_irq(USART2_IRQn);
}

/* ---- Commands ------------------------------------------------------------ */
static int led_from_name(const char *name)
{
    static const struct {
        const char *name;
        int pin;
    } leds[] = {{"green", LED_GREEN}, {"orange", LED_ORANGE}, {"red", LED_RED}, {"blue", LED_BLUE}};

    for (unsigned i = 0; i < sizeof leds / sizeof leds[0]; i++) {
        if (strcmp(name, leds[i].name) == 0) {
            return leds[i].pin;
        }
    }
    return -1;
}

static void cmd_help(void)
{
    printf("commands:\n"
           "  help                         this text\n"
           "  led <green|orange|red|blue> <on|off|toggle>\n"
           "  status                       LED states and UART statistics\n"
           "  uptime                       milliseconds since boot\n");
}

static void cmd_led(char *args)
{
    char *name = strtok(args, " ");
    char *action = strtok(NULL, " ");
    int pin = name ? led_from_name(name) : -1;

    if (pin < 0 || action == NULL) {
        printf("usage: led <green|orange|red|blue> <on|off|toggle>\n");
        return;
    }
    if (strcmp(action, "on") == 0) {
        led_on((uint32_t)pin);
    } else if (strcmp(action, "off") == 0) {
        led_off((uint32_t)pin);
    } else if (strcmp(action, "toggle") == 0) {
        led_toggle((uint32_t)pin);
    } else {
        printf("unknown action '%s'\n", action);
        return;
    }
    printf("%s is %s\n", name, (LED_PORT->ODR & BIT(pin)) ? "on" : "off");
}

static void cmd_status(void)
{
    uint32_t odr = LED_PORT->ODR;
    printf("LEDs: green=%d orange=%d red=%d blue=%d\n",
           !!(odr & BIT(LED_GREEN)), !!(odr & BIT(LED_ORANGE)),
           !!(odr & BIT(LED_RED)), !!(odr & BIT(LED_BLUE)));
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
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    systick_init(HSI_HZ);
    uart_rx_irq_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\nSTM32 CLI. Type 'help'.\n> ");

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
