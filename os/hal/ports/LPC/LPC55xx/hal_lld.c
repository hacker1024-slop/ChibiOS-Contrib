#include "hal.h"
#include "lpc55_clock.h"
#include "lpc_flash.h"

#define MAINCLKSELA LPC55_SYSCON(0x280u)
#define MAINCLKSELB LPC55_SYSCON(0x284u)
#define AHBCLKDIV LPC55_SYSCON(0x380u)
#define FMCCR LPC55_SYSCON(0x400u)
#define FRO192M_CTRL LPC55_ANACTRL(0x10u)

enum {
    ANALOG_CTRL_GATE = 1u << 27,
    FRO192M_POWER_DOWN = 1u << 5,
    FLASH_GATE = 1u << 7,
    FMC_GATE = 1u << 8,
    IOCON_GATE = 1u << 13,
    GPIO0_GATE = 1u << 14,
    GPIO1_GATE = 1u << 15,
    FRO12_ENABLE = 1u << 14,
    FRO96_ENABLE = 1u << 30,
    MAIN_A_FRO12 = 0,
    MAIN_A_FRO96 = 3,
    MAIN_B_FROM_A = 0,
    AHB_DIVIDE_BY_ONE = 0,
    FMCCR_PREFETCH = 1u << 5,
    FMCCR_FLASH_TIMING_SHIFT = 12,
    FMCCR_FLASH_TIMING_MASK = 0xfu << FMCCR_FLASH_TIMING_SHIFT,
    FLASH_READ_TIMING_96MHZ = 8,
};

_Static_assert(LPC55_CORE_CLOCK == LPC55_FRO96_HZ, "startup selects FRO96");

static void flash_set_read_timing(void) {
    /* The controller and FMC must use the same read timing. The command
       changes volatile timing fields while executing from SRAM. */
    uint32_t config = FMCCR;
    FMCCR = config & ~FMCCR_PREFETCH;
    if (!lpc_flash_set_read_timing(FLASH_READ_TIMING_96MHZ)) {
        __builtin_trap();
    }
    FMCCR = (config & ~FMCCR_FLASH_TIMING_MASK) |
            (FLASH_READ_TIMING_96MHZ << FMCCR_FLASH_TIMING_SHIFT);
}

/* Called at the SRAM entry point with interrupts masked. */
void lpc55_reset_interrupt_state(void) {
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    nvicInit();
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    __set_BASEPRI(0);
    __set_FAULTMASK(0);
    __DSB();
    __ISB();
}

void hal_lld_init(void) {
    nvicInit();
    LPC55_AHBCLKCTRLSET(2) = ANALOG_CTRL_GATE;
    LPC55_PRESETCTRLCLR(2) = ANALOG_CTRL_GATE;
    LPC55_PMC_PDRUNCFGCLR0 = FRO192M_POWER_DOWN;
    FRO192M_CTRL |= FRO12_ENABLE | FRO96_ENABLE;

    /* Slow the bus before replacing flash wait states or the AHB divider. */
    MAINCLKSELA = MAIN_A_FRO12;
    MAINCLKSELB = MAIN_B_FROM_A;
    LPC55_AHBCLKCTRLSET(0) = FLASH_GATE | FMC_GATE | IOCON_GATE | GPIO0_GATE | GPIO1_GATE;
    flash_set_read_timing();
    AHBCLKDIV = AHB_DIVIDE_BY_ONE;
    MAINCLKSELA = MAIN_A_FRO96;
}
