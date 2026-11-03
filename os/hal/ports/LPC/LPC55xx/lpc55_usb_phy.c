#include "lpc55_usb_phy.h"

#include <stdint.h>

#define REGISTER(address) (*(volatile uint32_t *)(address))

void lpc55_usb_phy_init(void) {
    enum {
        PORT_MODE = 0x400a3050u,
        PHY_PWD = 0x40038000u,
        PHY_CTRL_CLEAR = 0x40038038u,
        PHY_PLL = 0x400380a0u,
        PHY_PLL_SET = 0x400380a4u,
        PHY_PLL_CLEAR = 0x400380a8u,
    };

    REGISTER(PORT_MODE) = (1u << 19) | (1u << 16);
    REGISTER(PHY_PLL_CLEAR) = (1u << 20) | (1u << 16);
    REGISTER(PHY_PLL_SET) = (1u << 21) | (1u << 13) | (1u << 12);
    while (!(REGISTER(PHY_PLL) & (1u << 31))) {
    }
    REGISTER(PHY_PLL_SET) = 1u << 6;
    REGISTER(PHY_CTRL_CLEAR) = (1u << 31) | (1u << 30);
    REGISTER(PHY_PWD) = 0;
}
