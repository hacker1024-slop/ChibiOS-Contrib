#ifndef CMPARAMS_H
#define CMPARAMS_H

#define CORTEX_MODEL            33
#define CORTEX_HAS_FPU          1
#define CORTEX_PRIORITY_BITS    3
/* Highest LPC5528 IRQ is 59. The vector generator uses groups of eight. */
#define CORTEX_NUM_VECTORS      64
#define CORTEX_MPU_REGIONS      8

#if !defined(_FROM_ASM_)
#define __CM33_REV              0x0001U
#define __MPU_PRESENT           1U
#define __VTOR_PRESENT          1U
#define __NVIC_PRIO_BITS        CORTEX_PRIORITY_BITS
#define __Vendor_SysTickConfig  0U
#define __FPU_PRESENT           CORTEX_HAS_FPU
#define __DSP_PRESENT           1U
typedef enum {
    NonMaskableInt_IRQn = -14, HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12, BusFault_IRQn = -11, UsageFault_IRQn = -10,
    SVCall_IRQn = -5, DebugMonitor_IRQn = -4, PendSV_IRQn = -2, SysTick_IRQn = -1,
    CTIMER2_IRQn = 36, USB1_IRQn = 47
} IRQn_Type;
#include "core_cm33.h"
#endif

#endif
