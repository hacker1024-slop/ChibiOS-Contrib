#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

void st_lld_init(void);

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
#if OSAL_ST_RESOLUTION != 32
#error "The LPC5528 CTIMER system timer requires a 32-bit counter"
#endif
systime_t st_lld_get_counter(void);
void st_lld_start_alarm(systime_t abstime);
void st_lld_stop_alarm(void);
void st_lld_set_alarm(systime_t abstime);
systime_t st_lld_get_alarm(void);
bool st_lld_is_alarm_active(void);
#endif

#endif
