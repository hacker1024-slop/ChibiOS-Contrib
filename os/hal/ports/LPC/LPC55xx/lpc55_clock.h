#ifndef LPC55_CLOCK_H
#define LPC55_CLOCK_H

#include <stdint.h>

/* LPC5528 SYSCON, ANACTRL and PMC register offsets. */
#define LPC55_REG32(address) (*(volatile uint32_t *)(address))
#define LPC55_SYSCON(offset) LPC55_REG32(0x40000000u + (offset))
#define LPC55_ANACTRL(offset) LPC55_REG32(0x40013000u + (offset))
#define LPC55_PMC_PDRUNCFGCLR0 LPC55_REG32(0x400200c8u)
#define LPC55_PRESETCTRLSET(bank) LPC55_SYSCON(0x120u + 4u * (bank))
#define LPC55_PRESETCTRLCLR(bank) LPC55_SYSCON(0x140u + 4u * (bank))
#define LPC55_AHBCLKCTRLSET(bank) LPC55_SYSCON(0x220u + 4u * (bank))

#define LPC55_FRO96_HZ 96000000u

#endif
