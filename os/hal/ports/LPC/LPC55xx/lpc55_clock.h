#ifndef LPC55_CLOCK_H
#define LPC55_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

/* LPC5528 SYSCON, ANACTRL and PMC register offsets. */
#define LPC55_REG32(address) (*(volatile uint32_t *)(address))
#define LPC55_SYSCON(offset) LPC55_REG32(0x40000000u + (offset))
#define LPC55_ANACTRL(offset) LPC55_REG32(0x40013000u + (offset))
#define LPC55_PMC_PDRUNCFGCLR0 LPC55_REG32(0x400200c8u)
#define LPC55_PRESETCTRL(bank) LPC55_SYSCON(0x100u + 4u * (bank))
#define LPC55_PRESETCTRLSET(bank) LPC55_SYSCON(0x120u + 4u * (bank))
#define LPC55_PRESETCTRLCLR(bank) LPC55_SYSCON(0x140u + 4u * (bank))
#define LPC55_AHBCLKCTRLSET(bank) LPC55_SYSCON(0x220u + 4u * (bank))

#define LPC55_FRO96_HZ 96000000u

/* Read back each reset transition before accessing the peripheral. */
static inline bool lpc55_wait_reset(unsigned bank, uint32_t mask, uint32_t expected) {
    for (unsigned polls = 0; polls < 100000u; ++polls) {
        if ((LPC55_PRESETCTRL(bank) & mask) == expected) {
            return true;
        }
    }
    return false;
}

static inline bool lpc55_release_reset(unsigned bank, uint32_t mask) {
    LPC55_PRESETCTRLCLR(bank) = mask;
    return lpc55_wait_reset(bank, mask, 0);
}

static inline bool lpc55_pulse_reset(unsigned bank, uint32_t mask) {
    LPC55_PRESETCTRLSET(bank) = mask;
    return lpc55_wait_reset(bank, mask, mask) && lpc55_release_reset(bank, mask);
}

#endif
