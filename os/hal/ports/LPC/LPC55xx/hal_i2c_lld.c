#include "hal.h"

#if HAL_USE_I2C == TRUE

/* LPC5528 Flexcomm 4 register layout. CMSIS core headers have no I2C fields. */
#define FLEXCOMM4_BASE UINT32_C(0x4008a000)
#define FLEXCOMM4(offset) (*(volatile uint32_t *)(FLEXCOMM4_BASE + (offset)))
#define I2C_CFG FLEXCOMM4(0x800)
#define I2C_STAT FLEXCOMM4(0x804)
#define I2C_CLKDIV FLEXCOMM4(0x814)
#define I2C_MSTCTL FLEXCOMM4(0x820)
#define I2C_MSTTIME FLEXCOMM4(0x824)
#define I2C_MSTDAT FLEXCOMM4(0x828)
#define FLEXCOMM_PSELID FLEXCOMM4(0xff8)

enum {
    FLEXCOMM_SELECT_I2C = 3,
    CONFIG_MASTER_ENABLE = 1u << 0,
    MASTER_STATE_SHIFT = 1,
    MASTER_STATE_MASK = 7u << MASTER_STATE_SHIFT,
    MASTER_PENDING = 1u << 0,
    MASTER_ARBITRATION_LOST = 1 << 4,
    MASTER_START_STOP_ERROR = 1 << 6,
    MASTER_IDLE = 0,
    MASTER_RX_READY = 1,
    MASTER_TX_READY = 2,
    MASTER_NACK_ADDRESS = 3,
    MASTER_NACK_DATA = 4,
    MASTER_CONTINUE = 1,
    MASTER_START = 2,
    MASTER_STOP = 4,
    MASTER_SCL_HIGH_SHIFT = 4,
    MASTER_SCL_ENCODING_OFFSET = 2,
    MASTER_SCL_LOW_CLOCKS = 7,
    MASTER_SCL_HIGH_CLOCKS = 5,
    I2C_STANDARD_MODE_HZ = 100000,
    I2C_FAST_MODE_HZ = 400000,
    I2C_7BIT_ADDRESS_MAX = 0x7f,
    I2C_READ_BIT = 1,
    I2C_ADDRESS_SHIFT = 1,
};

I2CDriver I2CD4;

void i2c_lld_init(void) {
    i2cObjectInit(&I2CD4);
}

msg_t i2c_lld_start(I2CDriver *i2cp) {
    uint32_t speed = i2cp->config->clock_speed;
    if (speed != I2C_STANDARD_MODE_HZ && speed != I2C_FAST_MODE_HZ) {
        return HAL_RET_CONFIG_ERROR;
    }
    /* Use the same high/low counts in the divider and timing fields.
       The longer low phase meets the Fast-mode minimum low time at 400 kHz. */
    FLEXCOMM_PSELID = FLEXCOMM_SELECT_I2C;
    I2C_CLKDIV = LPC55_CORE_CLOCK / ((MASTER_SCL_LOW_CLOCKS + MASTER_SCL_HIGH_CLOCKS) * speed) - 1u;
    I2C_MSTTIME = ((MASTER_SCL_HIGH_CLOCKS - MASTER_SCL_ENCODING_OFFSET) << MASTER_SCL_HIGH_SHIFT) |
                  (MASTER_SCL_LOW_CLOCKS - MASTER_SCL_ENCODING_OFFSET);
    I2C_STAT = MASTER_ARBITRATION_LOST | MASTER_START_STOP_ERROR;
    I2C_CFG = CONFIG_MASTER_ENABLE;
    return HAL_RET_SUCCESS;
}

void i2c_lld_stop(I2CDriver *i2cp) {
    (void)i2cp;
    I2C_CFG = 0;
}

static msg_t wait_state(I2CDriver *i2cp, unsigned expected, systime_t start,
                        sysinterval_t timeout) {
    for (;;) {
        uint32_t status = I2C_STAT;
        if (status & MASTER_ARBITRATION_LOST) {
            i2cp->errors = I2C_ARBITRATION_LOST;
            return MSG_RESET;
        }
        if (status & MASTER_START_STOP_ERROR) {
            i2cp->errors = I2C_BUS_ERROR;
            return MSG_RESET;
        }
        if (status & MASTER_PENDING) {
            unsigned state = (status & MASTER_STATE_MASK) >> MASTER_STATE_SHIFT;
            if (state == expected) {
                return MSG_OK;
            }
            i2cp->errors = state == MASTER_NACK_ADDRESS || state == MASTER_NACK_DATA ?
                           I2C_ACK_FAILURE : I2C_BUS_ERROR;
            return MSG_RESET;
        }
        if (timeout != TIME_INFINITE && osalTimeDiffX(start, osalOsGetSystemTimeX()) >= timeout) {
            i2cp->errors = I2C_TIMEOUT;
            return MSG_TIMEOUT;
        }
    }
}

static msg_t transfer(I2CDriver *i2cp, i2caddr_t address, const uint8_t *tx, size_t tx_size,
                      uint8_t *rx, size_t rx_size, sysinterval_t timeout) {
    if (address > I2C_7BIT_ADDRESS_MAX) {
        i2cp->errors = I2C_ACK_FAILURE;
        return MSG_RESET;
    }

    /* The HAL calls with the kernel locked. Allow its clock to advance
       while polling, then return with the original lock convention. */
    osalSysUnlock();
    systime_t start = osalOsGetSystemTimeX();
    msg_t result = MSG_OK;
    if (tx_size != 0) {
        I2C_MSTDAT = address << I2C_ADDRESS_SHIFT;
        I2C_MSTCTL = MASTER_START;
        result = wait_state(i2cp, MASTER_TX_READY, start, timeout);
        for (size_t i = 0; i < tx_size && result == MSG_OK; ++i) {
            I2C_MSTDAT = tx[i];
            I2C_MSTCTL = MASTER_CONTINUE;
            result = wait_state(i2cp, MASTER_TX_READY, start, timeout);
        }
    }
    if (rx_size != 0 && result == MSG_OK) {
        I2C_MSTDAT = (address << I2C_ADDRESS_SHIFT) | I2C_READ_BIT;
        I2C_MSTCTL = MASTER_START;
        for (size_t i = 0; i < rx_size; ++i) {
            result = wait_state(i2cp, MASTER_RX_READY, start, timeout);
            if (result != MSG_OK) {
                break;
            }
            rx[i] = I2C_MSTDAT;
            if (i + 1 < rx_size) {
                I2C_MSTCTL = MASTER_CONTINUE;
            }
        }
    }
    if (i2cp->errors != I2C_ARBITRATION_LOST && (I2C_STAT & MASTER_PENDING)) {
        I2C_MSTCTL = MASTER_STOP;
        if (result == MSG_OK) {
            result = wait_state(i2cp, MASTER_IDLE, start, timeout);
        }
    }
    osalSysLock();
    return result;
}

msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t address,
                                      const uint8_t *tx, size_t tx_size,
                                      uint8_t *rx, size_t rx_size, sysinterval_t timeout) {
    return transfer(i2cp, address, tx, tx_size, rx, rx_size, timeout);
}

msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t address,
                                     uint8_t *rx, size_t rx_size, sysinterval_t timeout) {
    return transfer(i2cp, address, NULL, 0, rx, rx_size, timeout);
}

#endif
