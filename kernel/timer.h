#ifndef SB_KERNEL_TIMER_H
#define SB_KERNEL_TIMER_H

#include <stdint.h>

struct sb_timer_saved_gpr;

/* Default kernel timer used by the bootstrap path. */
void timer_init(void);
/* Configurable PIT timer for later policy/runtime use. */
void timer_init_frequency(uint32_t frequency_hz);
uint64_t timer_ticks(void);
void sb_timer_tick(void);
uintptr_t sb_timer_irq_dispatch(struct sb_timer_saved_gpr *gpr);

#endif