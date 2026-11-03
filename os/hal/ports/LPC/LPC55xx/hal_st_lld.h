#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
#error "LPC55xx currently requires periodic ChibiOS system ticks"
#endif

void st_lld_init(void);

#endif
