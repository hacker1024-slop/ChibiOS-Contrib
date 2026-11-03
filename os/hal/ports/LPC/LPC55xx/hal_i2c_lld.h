#ifndef HAL_I2C_LLD_H
#define HAL_I2C_LLD_H

typedef uint16_t i2caddr_t;
typedef uint32_t i2cflags_t;

typedef struct {
    uint32_t clock_speed;
} I2CConfig;

typedef struct hal_i2c_driver {
    i2cstate_t state;
    const I2CConfig *config;
    i2cflags_t errors;
#if I2C_USE_MUTUAL_EXCLUSION
    mutex_t mutex;
#endif
#ifdef I2C_DRIVER_EXT_FIELDS
    I2C_DRIVER_EXT_FIELDS
#endif
} I2CDriver;

#define I2C_LLD_ENHANCED_API
#define i2c_lld_get_errors(i2cp) ((i2cp)->errors)

extern I2CDriver I2CD4;

void i2c_lld_init(void);
msg_t i2c_lld_start(I2CDriver *i2cp);
void i2c_lld_stop(I2CDriver *i2cp);
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t address,
                                      const uint8_t *tx, size_t tx_size,
                                      uint8_t *rx, size_t rx_size, sysinterval_t timeout);
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t address,
                                     uint8_t *rx, size_t rx_size, sysinterval_t timeout);

#endif
