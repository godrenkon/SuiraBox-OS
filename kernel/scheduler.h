#ifndef SB_KERNEL_SCHEDULER_H
#define SB_KERNEL_SCHEDULER_H

#include <stdint.h>

typedef enum {
    SB_TASK_UNUSED = 0,
    SB_TASK_READY,
    SB_TASK_RUNNING,
    SB_TASK_BLOCKED,
    SB_TASK_SLEEPING,
    SB_TASK_EXITED
} sb_task_state_t;

/* Saved kernel execution context used by the architecture context switcher.
 * rsp points at the switch-return stack position and rip is the return address
 * that must be resumed when the task becomes current again. */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rsp;
} sb_task_context_t;

typedef struct {
    uint64_t id;
    uint64_t runtime_ticks;
    uint32_t priority;
    sb_task_state_t state;
    sb_task_context_t context;
    uint64_t kernel_stack_base;
    uint64_t kernel_stack_top;
} sb_task_t;

void scheduler_init(void);
void scheduler_tick(void);
uint64_t scheduler_ticks(void);
sb_task_t *scheduler_current(void);

int scheduler_add_kernel_task(uint64_t id, uint32_t priority);
sb_task_t *scheduler_pick_next(void);
uint32_t scheduler_task_count(void);

/* Cooperative switch primitive. Interrupt masking and address-space changes
 * remain the caller's responsibility until preemptive scheduling is enabled. */
void scheduler_switch_to(sb_task_t *next);

#endif
