#include "lpc55_i2c.h"

#define FLEXCOMM4(offset) (*(volatile uint32_t *)(0x4008a000u + (offset)))
#define I2C_CFG FLEXCOMM4(0x800)
#define I2C_STAT FLEXCOMM4(0x804)
#define I2C_CLKDIV FLEXCOMM4(0x814)
#define I2C_MSTCTL FLEXCOMM4(0x820)
#define I2C_MSTTIME FLEXCOMM4(0x824)
#define I2C_MSTDAT FLEXCOMM4(0x828)
#define FLEXCOMM_PSELID FLEXCOMM4(0xff8)

enum {
    MASTER_PENDING = 1,
    MASTER_TX_READY = 2,
    MASTER_CONTINUE = 1,
    MASTER_START = 2,
    MASTER_STOP = 4,
};

void lpc55_i2c_init(void) {
    FLEXCOMM_PSELID = 3;
    I2C_CLKDIV = 23;
    I2C_MSTTIME = 0x44;
    I2C_CFG = 1;
}

static lpc55_i2c_status_t wait_transmit(uint32_t start, uint16_t timeout, uint32_t (*now)(void)) {
    uint32_t status;
    do {
        status = I2C_STAT;
        if (status & MASTER_PENDING) {
            return ((status >> 1) & 7) == MASTER_TX_READY ? LPC55_I2C_SUCCESS : LPC55_I2C_ERROR;
        }
    } while ((uint32_t)(now() - start) < timeout);
    return LPC55_I2C_TIMEOUT;
}

lpc55_i2c_status_t lpc55_i2c_transmit(uint8_t address, const uint8_t *prefix, uint16_t prefix_length,
                             const uint8_t *data, uint16_t length, uint16_t timeout, uint32_t (*now)(void)) {
    uint32_t start = now();
    I2C_MSTDAT = address & 0xfe;
    I2C_MSTCTL = MASTER_START;
    lpc55_i2c_status_t status = wait_transmit(start, timeout, now);
    for (uint16_t i = 0; i < prefix_length && status == LPC55_I2C_SUCCESS; ++i) {
        I2C_MSTDAT = prefix[i];
        I2C_MSTCTL = MASTER_CONTINUE;
        status = wait_transmit(start, timeout, now);
    }
    for (uint16_t i = 0; i < length && status == LPC55_I2C_SUCCESS; ++i) {
        I2C_MSTDAT = data[i];
        I2C_MSTCTL = MASTER_CONTINUE;
        status = wait_transmit(start, timeout, now);
    }
    I2C_MSTCTL = MASTER_STOP;
    return status;
}
