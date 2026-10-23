/*
 * Lesson 17: I2C bus and a Si7021 temperature / humidity sensor.
 *
 *   1. scan the bus: which 7-bit addresses acknowledge?
 *   2. read the sensor's electronic ID
 *   3. measure temperature and humidity
 *
 * stimulus.resc changes the simulated sensor's readings half way through.
 */
#include <stdio.h>

#include "board.h"
#include "i2c.h"
#include "tick.h"
#include "uart.h"

#define SI7021_ADDR 0x40u

/* Si7021 commands (see the I2C command table in its datasheet) */
#define CMD_MEASURE_RH_HOLD   0xE5u
#define CMD_MEASURE_TEMP_HOLD 0xE3u

/*
 * Send a measure command, then read the 16-bit result (MSB first). The sensor
 * would append a CRC-8 byte; NACKing after the LSB tells it to skip that.
 */
static int32_t si7021_measure(uint8_t cmd)
{
    uint8_t buf[2];
    if (i2c1_write(SI7021_ADDR, &cmd, 1) != I2C_OK) {
        return -1;
    }
    /* "Hold master" commands: the sensor stretches SCL until the result is ready. */
    if (i2c1_read(SI7021_ADDR, buf, sizeof buf) != I2C_OK) {
        return -1;
    }
    return (int32_t)((buf[0] << 8) | buf[1]);
}

/* Datasheet formulas in hundredths (no float in printf), rounded to nearest:
 *   T  = 175.72 * code / 65536 - 46.85
 *   RH = 125 * code / 65536 - 6            */
static int32_t temp_centi(int32_t code) { return (17572 * code + 32768) / 65536 - 4685; }
static int32_t rh_centi(int32_t code)   { return (12500 * code + 32768) / 65536 - 600; }

static void print_centi(const char *label, int32_t v, const char *unit)
{
    int32_t mag = v < 0 ? -v : v;
    printf("%s%s%ld.%02ld %s", label, v < 0 ? "-" : "", (long)(mag / 100), (long)(mag % 100), unit);
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    i2c1_init(HSI_HZ);

    printf("\nlesson 17: I2C1 at 100 kHz (FREQ=%lu, CCR=%lu, TRISE=%lu)\n",
           (unsigned long)(HSI_HZ / 1000000u), (unsigned long)(HSI_HZ / 200000u),
           (unsigned long)(HSI_HZ / 1000000u + 1u));

    printf("bus scan:");
    int found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c1_probe(addr) == I2C_OK) {
            printf(" 0x%02x", addr);
            found++;
        }
    }
    printf("  (%d device%s)\n", found, found == 1 ? "" : "s");

    /* Electronic ID, second half: command 0xFC 0xC9, first byte is SNB_3 = device type. */
    uint8_t id_cmd[2] = {0xFC, 0xC9};
    uint8_t id[6];
    if (i2c1_write(SI7021_ADDR, id_cmd, 2) == I2C_OK && i2c1_read(SI7021_ADDR, id, sizeof id) == I2C_OK) {
        printf("Si70xx device type: 0x%02x (%s)\n", id[0], id[0] == 0x15 ? "Si7021" : "other");
    } else {
        printf("ID read failed\n");
    }

    for (int i = 0; i < 4; i++) {
        int32_t t = si7021_measure(CMD_MEASURE_TEMP_HOLD);
        int32_t h = si7021_measure(CMD_MEASURE_RH_HOLD);
        printf("t=%3lu ms  ", (unsigned long)millis());
        if (t < 0 || h < 0) {
            printf("measurement failed\n");
        } else {
            print_centi("temperature ", temp_centi(t), "C");
            print_centi("   humidity ", rh_centi(h), "%RH\n");
        }
        led_toggle(LED_GREEN);
        delay_ms(100);
    }

    printf("probe 0x50 (nothing there): %s\n", i2c1_probe(0x50) == I2C_ERR_NACK ? "NACK" : "?");
    for (;;) {
        __WFI();
    }
}
