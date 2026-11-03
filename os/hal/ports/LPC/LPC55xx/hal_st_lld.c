#include "hal.h"

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
#include "lpc55_systick.h"

#ifndef LPC55_ST_IRQ_PRIORITY
#define LPC55_ST_IRQ_PRIORITY 6
#endif

_Static_assert(LPC55_CORE_CLOCK % OSAL_ST_FREQUENCY == 0, "nonintegral system tick");
_Static_assert(LPC55_CORE_CLOCK / OSAL_ST_FREQUENCY - 1u <= SysTick_LOAD_RELOAD_Msk, "SysTick reload overflow");

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

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING

#ifndef LPC55_ST_IRQ_PRIORITY
#define LPC55_ST_IRQ_PRIORITY 6
#endif
#define LPC55_CTIMER2_CLOCK 96000000u
#define SYSCON_CTIMER2_CLKSEL (*(volatile uint32_t *)UINT32_C(0x40000274))
#define SYSCON_AHBCLKCTRLSET1 (*(volatile uint32_t *)UINT32_C(0x40000224))

#define CTIMER2_REG(offset) (*(volatile uint32_t *)(UINT32_C(0x40028000) + (offset)))
#define CTIMER_IR CTIMER2_REG(0x00)
#define CTIMER_TCR CTIMER2_REG(0x04)
#define CTIMER_TC CTIMER2_REG(0x08)
#define CTIMER_PR CTIMER2_REG(0x0c)
#define CTIMER_MCR CTIMER2_REG(0x14)
#define CTIMER_MR1 CTIMER2_REG(0x1c)
#define CTIMER_CTCR CTIMER2_REG(0x70)

enum {
    CTIMER2_CLOCK_GATE = 1u << 22,
    CTIMER_CLOCK_FRO96 = 3u,
    CTIMER_ENABLE = 1u,
    CTIMER_RESET = 2u,
    CTIMER_MATCH1_FLAG = 1u << 1,
    CTIMER_MATCH1_INTERRUPT = 1u << 3,
};

_Static_assert(LPC55_CTIMER2_CLOCK % OSAL_ST_FREQUENCY == 0, "nonintegral system tick");

OSAL_IRQ_HANDLER(VectorD0) {
    OSAL_IRQ_PROLOGUE();
    if (CTIMER_IR & CTIMER_MATCH1_FLAG) {
        CTIMER_IR = CTIMER_MATCH1_FLAG;
        osalSysLockFromISR();
        osalOsTimerHandlerI();
        osalSysUnlockFromISR();
    }
    OSAL_IRQ_EPILOGUE();
}

void st_lld_init(void) {
    SYSCON_CTIMER2_CLKSEL = CTIMER_CLOCK_FRO96;
    SYSCON_AHBCLKCTRLSET1 = CTIMER2_CLOCK_GATE;
    CTIMER_TCR = CTIMER_RESET;
    CTIMER_CTCR = 0;
    CTIMER_PR = LPC55_CTIMER2_CLOCK / OSAL_ST_FREQUENCY - 1u;
    CTIMER_MCR = 0;
    CTIMER_IR = CTIMER_MATCH1_FLAG;
    CTIMER_TCR = CTIMER_ENABLE;
    nvicEnableVector(CTIMER2_IRQn, LPC55_ST_IRQ_PRIORITY);
}

systime_t st_lld_get_counter(void) {
    return CTIMER_TC;
}

void st_lld_start_alarm(systime_t abstime) {
    CTIMER_MR1 = abstime;
    CTIMER_IR = CTIMER_MATCH1_FLAG;
    CTIMER_MCR = CTIMER_MATCH1_INTERRUPT;
}

void st_lld_stop_alarm(void) {
    CTIMER_MCR = 0;
    CTIMER_IR = CTIMER_MATCH1_FLAG;
}

void st_lld_set_alarm(systime_t abstime) {
    CTIMER_MR1 = abstime;
}

systime_t st_lld_get_alarm(void) {
    return CTIMER_MR1;
}

bool st_lld_is_alarm_active(void) {
    return (CTIMER_MCR & CTIMER_MATCH1_INTERRUPT) != 0;
}

#endif
