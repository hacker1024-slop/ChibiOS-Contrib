#pragma once

#include "cmparams.h"


static inline void lpc55_systick_start(uint32_t cycles) {
    SysTick->LOAD = cycles - 1u;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

static inline uint32_t lpc55_systick_current(void) {
    return SysTick->VAL;
}
