#include "hal.h"
#include "lpc55_clock.h"
#include "lpc55_usb_phy.h"

#define USB_PORT_MODE LPC55_REG32(0x400a3050u)
#define USB_PHY_PWD LPC55_REG32(0x40038000u)
#define USB_PHY_CTRL_CLEAR LPC55_REG32(0x40038038u)
#define USB_PHY_PLL LPC55_REG32(0x400380a0u)
#define USB_PHY_PLL_SET LPC55_REG32(0x400380a4u)
#define USB_PHY_PLL_CLEAR LPC55_REG32(0x400380a8u)
#define CLOCK_CTRL LPC55_SYSCON(0xa18u)
#define XO32M_CTRL LPC55_ANACTRL(0x20u)

enum {
    USB1_HOST = 1u << 4,
    USB1_DEVICE = 1u << 5,
    USB1_RAM = 1u << 6,
    USB1_PHY = 1u << 7,
    USB1_CLOCKS = USB1_HOST | USB1_DEVICE | USB1_RAM | USB1_PHY,
    XTAL32M_POWER_DOWN = 1u << 8,
    USBHS_PHY_POWER_DOWN = 1u << 12,
    USBHS_LDO_POWER_DOWN = 1u << 18,
    XTAL32M_LDO_POWER_DOWN = 1u << 20,
    CLKIN_ENABLE = 1u << 5,
    XO32M_USB_OUTPUT_ENABLE = 1u << 23,
    PORT_DEVICE_ENABLE = 1u << 16,
    PORT_SW_PDCOM = 1u << 19,
    PHY_RESET = 1u << 31,
    PHY_CLOCK_GATE = 1u << 30,
    PLL_USB_CLOCK_ENABLE = 1u << 6,
    PLL_POWER = 1u << 12,
    PLL_ENABLE = 1u << 13,
    PLL_RESERVED_CLEAR = 1u << 16,
    PLL_BIAS_POWER_DOWN = 1u << 20,
    PLL_REGULATOR_ENABLE = 1u << 21,
    PLL_DIVIDER_MASK = 7u << 22,
    PLL_DIVIDE_BY_20 = 3u << 22,
    PLL_PREDIV = 1u << 30,
    PLL_LOCK = 1u << 31,
    PHY_POWER_SETTLE_NOPS = 100000u,
    PLL_LOCK_POLL_LIMIT = 1000000u,
};

void lpc55_usb_phy_init(void) {
    LPC55_PMC_PDRUNCFGCLR0 = XTAL32M_POWER_DOWN | XTAL32M_LDO_POWER_DOWN |
                            USBHS_PHY_POWER_DOWN | USBHS_LDO_POWER_DOWN;
    CLOCK_CTRL |= CLKIN_ENABLE;
    XO32M_CTRL |= XO32M_USB_OUTPUT_ENABLE;
    LPC55_AHBCLKCTRLSET(2) = USB1_CLOCKS;
    if (!lpc55_pulse_reset(2, USB1_CLOCKS)) {
        osalSysHalt("USB1 peripheral reset timeout");
    }

    /* Allow the analog supplies to settle before releasing PHY soft reset. */
    for (unsigned n = 0; n < PHY_POWER_SETTLE_NOPS; ++n) {
        __NOP();
    }

    USB_PORT_MODE = PORT_SW_PDCOM | PORT_DEVICE_ENABLE;
    USB_PHY_CTRL_CLEAR = PHY_RESET;
    USB_PHY_PLL_CLEAR = PLL_USB_CLOCK_ENABLE | PLL_POWER | PLL_ENABLE |
                        PLL_BIAS_POWER_DOWN | PLL_RESERVED_CLEAR |
                        PLL_DIVIDER_MASK | PLL_PREDIV;
    /* The board uses the divide-by-20 setting observed at stock handoff. */
    USB_PHY_PLL_SET = PLL_DIVIDE_BY_20 | PLL_REGULATOR_ENABLE;
    USB_PHY_PLL_SET = PLL_ENABLE | PLL_POWER;

    for (unsigned polls = PLL_LOCK_POLL_LIMIT; !(USB_PHY_PLL & PLL_LOCK); --polls) {
        if (polls == 0) {
            osalSysHalt("USB PHY PLL lock timeout");
        }
    }
    USB_PHY_PLL_SET = PLL_USB_CLOCK_ENABLE;
    USB_PHY_CTRL_CLEAR = PHY_CLOCK_GATE;
    USB_PHY_PWD = 0;
}
