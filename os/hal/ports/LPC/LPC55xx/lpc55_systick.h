#pragma once

#include <stdint.h>


static inline void lpc55_systick_start(uint32_t cycles) {
    volatile uint32_t *const registers = (volatile uint32_t *)0xe000e010u;
    registers[1] = cycles - 1;
    registers[2] = 0;
    registers[0] = 7;
}

static inline uint32_t lpc55_systick_current(void) {
    return *(volatile uint32_t *)0xe000e018u;
}
