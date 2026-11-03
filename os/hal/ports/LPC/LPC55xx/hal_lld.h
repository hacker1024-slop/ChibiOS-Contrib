#ifndef HAL_LLD_H
#define HAL_LLD_H

#include "cmparams.h"
#include "nvic.h"

/* The board supplies the core clock after the SRAM bootloader handoff. */
#ifndef LPC55_CORE_CLOCK
#define LPC55_CORE_CLOCK 96000000u
#endif

#define PLATFORM_NAME "NXP LPC55xx"
#define hal_lld_get_clock_point(clkpt) (0u)

#ifdef __cplusplus
extern "C" {
#endif
void hal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif
