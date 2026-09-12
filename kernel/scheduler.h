#ifndef SB_KERNEL_SCHEDULER_H
#define SB_KERNEL_SCHEDULER_H

#include <stdint.h>
#include "arch/x86_64/irq_frame.h"

typedef enum {
    SB_TASK_UNUSED = 0,
    SB_TASK_READY,
    SB_TASK_RUNNING,
    SB_TASK_BLOCKED,
    SB_TASK_SLEEPING,
    SB_TASK_EXITED
} sb_task_state_t;

/* Saved cooperative kernel execution context. */
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
    uint64_t process_id;
    uint64_t runtime_ticks;
    uint64_t dispatch_count;
    uint64_t wake_tick;
    uint64_t block_timeout_result;
    int64_t exit_code;
    uint32_t priority;
    sb_task_state_t state;
    sb_task_context_t context;
    uint64_t irq_frame_rsp;
    uint64_t address_space_cr3;
    uint64_t kernel_stack_base;
    uint64_t kernel_stack_top;
    uint8_t user_task;
    uint8_t block_timeout_armed;
} sb_task_t;

void scheduler_init(void);
void scheduler_tick(void);
uint64_t scheduler_ticks(void);
sb_task_t *scheduler_current(void);

int scheduler_add_kernel_task(uint64_t id, uint32_t priority);
int scheduler_add_user_task(uint64_t id,
                            uint64_t process_id,
                            uint32_t priority,
                            uint64_t address_space_cr3,
                            uint64_t user_entry,
                            uint64_t user_stack_top);

/* Selects a runnable task without changing scheduler state. */
sb_task_t *scheduler_pick_next(void);
uint32_t scheduler_task_count(void);

/* State transitions for the current single-CPU scheduler. Blocking/sleeping
 * callers must reschedule before returning to code that belongs to the task. */
int scheduler_block_current(void);
/* Block the current user task until another object wakes it or delay_ticks
 * expires. On timeout the scheduler writes timeout_result into the task's saved
 * syscall-frame RAX before making the task runnable again. */
int scheduler_block_current_until(uint64_t delay_ticks, uint64_t timeout_result);
int scheduler_sleep_current(uint64_t delay_ticks);
int scheduler_wake_task(uint64_t id);
/* Complete a blocked syscall by writing its eventual RAX result into the saved
 * frame before making the task runnable again. */
int scheduler_wake_task_with_result(uint64_t id, uint64_t result);
/* Returns 1 only while the identified live task is BLOCKED, 0 otherwise. */
int scheduler_task_is_blocked(uint64_t id);

/* Process exit is two-phase. This marks all tasks in the current user process
 * EXITED. Their stacks are freed later from a different scheduler context. */
int scheduler_exit_current_process(int64_t exit_code);
/* Reclaim EXITED task slots and kernel stacks for pid. Never reaps the current
 * process. Returns a non-negative number of task slots reclaimed. */
int scheduler_reap_process(uint64_t pid);

/* Save current_frame and immediately choose another runnable execution frame.
 * This is used by blocking/exit syscalls where waiting until the next timer
 * quantum would violate the syscall semantics. */
sb_irq_frame_t *scheduler_reschedule(sb_irq_frame_t *current_frame);

/* Called from the timer IRQ with a complete register frame. The returned frame
 * is the frame that the IRQ epilogue must restore before iretq. */
sb_irq_frame_t *scheduler_preempt(sb_irq_frame_t *current_frame);

/* Cooperative kernel switch primitive. This path is separate from IRQ-driven
 * preemption and intentionally keeps the smaller callee-saved context ABI. */
void scheduler_switch_to(sb_task_t *next);

#endif