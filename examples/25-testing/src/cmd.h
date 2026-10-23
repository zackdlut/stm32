/*
 * Text command interpreter. It reaches the hardware only through cmd_hw_t,
 * so a test can pass fakes and check what the commands did.
 */
#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { CMD_LED_GREEN, CMD_LED_ORANGE, CMD_LED_RED, CMD_LED_BLUE };

typedef struct {
    void (*led_set)(int led, bool on);
    uint32_t (*millis)(void);
} cmd_hw_t;

/* Runs one line. Writes the reply (no trailing newline) into out.
 * Returns 0 on success, -1 for an unknown command or bad arguments. */
int cmd_execute(const char *line, char *out, size_t out_len, const cmd_hw_t *hw);

#endif
