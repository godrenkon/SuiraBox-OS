#ifndef SB_KERNEL_TIMER_H
#define SB_KERNEL_TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency_hz);
uint64_t timer_ticks(void);
void sb_timer_tick(void);

#endif
