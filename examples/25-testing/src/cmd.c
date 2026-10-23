#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "baud.h"

#define MAX_LINE 64
#define MAX_ARGS 4

static const char *const led_names[] = {"green", "orange", "red", "blue"};

static int split(char *s, char *argv[])
{
    int argc = 0;
    for (char *tok = strtok(s, " \t"); tok && argc < MAX_ARGS; tok = strtok(NULL, " \t")) {
        argv[argc++] = tok;
    }
    return argc;
}

static bool parse_u32(const char *s, uint32_t *value)
{
    char *end;
    unsigned long v = strtoul(s, &end, 10);
    if (*s == '\0' || *s == '-' || *end != '\0' || v > UINT32_MAX) {
        return false;
    }
    *value = (uint32_t)v;
    return true;
}

static int cmd_led(int argc, char *argv[], char *out, size_t n, const cmd_hw_t *hw)
{
    if (argc == 3) {
        for (int led = 0; led < 4; led++) {
            if (strcmp(argv[1], led_names[led]) != 0) {
                continue;
            }
            if (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "off") == 0) {
                hw->led_set(led, argv[2][1] == 'n');
                snprintf(out, n, "ok");
                return 0;
            }
        }
    }
    snprintf(out, n, "error: usage: led <green|orange|red|blue> <on|off>");
    return -1;
}

static int cmd_baud(int argc, char *argv[], char *out, size_t n)
{
    uint32_t pclk, baud;
    if (argc != 3 || !parse_u32(argv[1], &pclk) || !parse_u32(argv[2], &baud) || baud == 0 ||
        pclk / baud < 16u) {
        snprintf(out, n, "error: usage: baud <pclk_hz> <baud>, pclk_hz >= 16 * baud");
        return -1;
    }
    uint32_t brr = uart_brr(pclk, baud);
    snprintf(out, n, "BRR=%lu (0x%04lx), actual %lu baud, error %ld ppm", (unsigned long)brr,
             (unsigned long)brr, (unsigned long)uart_baud_from_brr(pclk, brr),
             (long)uart_baud_error_ppm(pclk, baud));
    return 0;
}

int cmd_execute(const char *line, char *out, size_t out_len, const cmd_hw_t *hw)
{
    char copy[MAX_LINE];
    char *argv[MAX_ARGS];

    if (strlen(line) >= sizeof copy) {
        snprintf(out, out_len, "error: line too long");
        return -1;
    }
    strcpy(copy, line);
    int argc = split(copy, argv);
    if (argc == 0) {
        out[0] = '\0';
        return 0;
    }

    if (strcmp(argv[0], "help") == 0) {
        snprintf(out, out_len, "commands: help, led <color> <on|off>, baud <pclk_hz> <baud>, uptime");
        return 0;
    }
    if (strcmp(argv[0], "led") == 0) {
        return cmd_led(argc, argv, out, out_len, hw);
    }
    if (strcmp(argv[0], "baud") == 0) {
        return cmd_baud(argc, argv, out, out_len);
    }
    if (strcmp(argv[0], "uptime") == 0) {
        snprintf(out, out_len, "uptime %lu ms", (unsigned long)hw->millis());
        return 0;
    }
    snprintf(out, out_len, "error: unknown command '%s'", argv[0]);
    return -1;
}
