#include "hal.h"

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
#include "lpc55_systick.h"

#ifndef LPC55_ST_IRQ_PRIORITY
#define LPC55_ST_IRQ_PRIORITY 6
#endif

_Static_assert(LPC55_CORE_CLOCK % OSAL_ST_FREQUENCY == 0, "nonintegral system tick");
_Static_assert(LPC55_CORE_CLOCK / OSAL_ST_FREQUENCY <= (1u << 24), "SysTick reload overflow");

OSAL_IRQ_HANDLER(SysTick_Handler) {
    OSAL_IRQ_PROLOGUE();
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
    OSAL_IRQ_EPILOGUE();
}

void st_lld_init(void) {
    NVIC_SetPriority(SysTick_IRQn, LPC55_ST_IRQ_PRIORITY);
    lpc55_systick_start(LPC55_CORE_CLOCK / OSAL_ST_FREQUENCY);
}
#endif
