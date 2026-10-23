/*
 * Polled I2C1 master on PB6 (SCL) / PB7 (SDA), standard mode 100 kHz.
 * Every function returns 0 on success and a negative i2c_err_t on failure.
 */
#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_NACK = -1,      /* no device acknowledged the address */
    I2C_ERR_TIMEOUT = -2,   /* a flag never came: bus stuck or wrong clock */
} i2c_err_t;

void i2c1_init(uint32_t pclk1_hz);
int i2c1_write(uint8_t addr7, const uint8_t *data, size_t len);
int i2c1_read(uint8_t addr7, uint8_t *data, size_t len);
int i2c1_probe(uint8_t addr7);

#endif
