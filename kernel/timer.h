#ifndef SB_KERNEL_TIMER_H
#define SB_KERNEL_TIMER_H

#include <stdint.h>
#include "arch/x86_64/irq_frame.h"

void timer_init(uint32_t frequency_hz);
uint64_t timer_ticks(void);
sb_irq_frame_t *sb_timer_tick(sb_irq_frame_t *frame);

#endif
