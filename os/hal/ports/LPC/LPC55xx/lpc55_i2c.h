#pragma once

#include <stdint.h>

typedef enum {
    LPC55_I2C_SUCCESS = 0,
    LPC55_I2C_ERROR = -1,
    LPC55_I2C_TIMEOUT = -2,
} lpc55_i2c_status_t;

void lpc55_i2c_init(void);
lpc55_i2c_status_t lpc55_i2c_transmit(uint8_t address, const uint8_t *prefix,
                                    uint16_t prefix_length, const uint8_t *data,
                                    uint16_t length, uint16_t timeout, uint32_t (*now)(void));
