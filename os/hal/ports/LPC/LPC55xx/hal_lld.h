#ifndef HAL_LLD_H
#define HAL_LLD_H

#include "cmparams.h"
#include "nvic.h"

/* The LPC5528 HAL selects the 96 MHz FRO for the system clock. */
#ifndef LPC55_CORE_CLOCK
#define LPC55_CORE_CLOCK 96000000u
#endif

#define PLATFORM_NAME "NXP LPC55xx"
#define hal_lld_get_clock_point(clkpt) (0u)

#ifdef __cplusplus
extern "C" {
#endif
void hal_lld_init(void);
void lpc55_reset_interrupt_state(void);
#ifdef __cplusplus
}
#endif

#endif
