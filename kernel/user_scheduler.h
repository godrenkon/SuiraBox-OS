#ifndef SB_KERNEL_USER_SCHEDULER_H
#define SB_KERNEL_USER_SCHEDULER_H

#include <stdint.h>
#include "process.h"
#include "arch/x86_64/irq_frame.h"

#define SB_MAX_USER_SCHED_THREADS 32u
#define SB_USER_SCHED_QUANTUM_TICKS 10u

void user_scheduler_init(void);
int user_scheduler_add(sb_process_t *process, sb_thread_t *thread);
int user_scheduler_remove(sb_process_t *process, sb_thread_t *thread);
int user_scheduler_rebind_thread(sb_process_t *process, sb_thread_t *old_thread, sb_thread_t *new_thread);
int user_scheduler_set_current(sb_process_t *process, sb_thread_t *thread);
sb_thread_t *user_scheduler_current_thread(void);
sb_process_t *user_scheduler_current_process(void);
uint32_t user_scheduler_count(void);

/* Returns the %rsp value the timer IRQ stub must restore. */
uintptr_t user_scheduler_timer_dispatch(sb_timer_saved_gpr_t *gpr);

#endif
