#ifndef HAL_PAL_LLD_H
#define HAL_PAL_LLD_H

#include "lpc55_gpio.h"

typedef uint32_t ioportmask_t;
typedef uint32_t iomode_t;
typedef uint32_t ioline_t;
typedef uint32_t ioportid_t;
typedef uint32_t iopadid_t;
typedef uint32_t ioeventmode_t;

#define PAL_IOPORTS_WIDTH 32
#define PAL_WHOLE_PORT UINT32_MAX
#define PAL_LINE(port, pad) (((port) << 5) | (pad))
#define PAL_PORT(line) ((line) >> 5)
#define PAL_PAD(line) ((line) & 31u)
#define PAL_NOLINE UINT32_MAX
#define GPIOA 0u
#define GPIOB 1u


#define PAL_NEW_INIT
#define pal_lld_init() ((void)0)
#define pal_lld_readport(port) LPC_REG(0x4008e100u + 4u * (port))
#define pal_lld_readlatch(port) pal_lld_readport(port)
#define pal_lld_writeport(port, bits) (pal_lld_readport(port) = (bits))
#define pal_lld_setport(port, bits) (LPC_REG(0x4008e200u + 4u * (port)) = (bits))
#define pal_lld_clearport(port, bits) (LPC_REG(0x4008e280u + 4u * (port)) = (bits))
#define pal_lld_toggleport(port, bits) (LPC_REG(0x4008e300u + 4u * (port)) = (bits))
#define pal_lld_setpadmode(port, pad, mode) lpc55_pal_mode(PAL_LINE(port, pad), mode)

static inline void lpc55_pal_mode(ioline_t line, iomode_t mode) {
    if (mode == PAL_MODE_OUTPUT_PUSHPULL || mode == PAL_MODE_OUTPUT_OPENDRAIN) {
        lpc_gpio_output(line);
        if (mode == PAL_MODE_OUTPUT_OPENDRAIN) {
            LPC_REG(0x40001000u + 4u * line) |= 1u << 9;
        }
    } else {
        uint32_t pull = mode == PAL_MODE_INPUT_PULLUP ? 2u :
                        mode == PAL_MODE_INPUT_PULLDOWN ? 1u : 0u;
        lpc_gpio_input(line, pull);
        if (mode == PAL_MODE_INPUT_ANALOG) {
            LPC_REG(0x40001000u + 4u * line) &= ~(1u << 8);
        }
    }
}

#endif
