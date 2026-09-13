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
/* Same synthetic ring3-frame creation as scheduler_add_user_task(), with the
 * first SysV argument register initialized before the task becomes runnable. */
int scheduler_add_user_task_arg(uint64_t id,
                                uint64_t process_id,
                                uint32_t priority,
                                uint64_t address_space_cr3,
                                uint64_t user_entry,
                                uint64_t user_stack_top,
                                uint64_t initial_rdi);

sb_task_t *scheduler_pick_next(void);
uint32_t scheduler_task_count(void);

int scheduler_block_current(void);
int scheduler_block_current_until(uint64_t delay_ticks, uint64_t timeout_result);
int scheduler_sleep_current(uint64_t delay_ticks);
int scheduler_wake_task(uint64_t id);
int scheduler_wake_task_with_result(uint64_t id, uint64_t result);
int scheduler_task_is_blocked(uint64_t id);

int scheduler_exit_current_process(int64_t exit_code);
int scheduler_reap_process(uint64_t pid);

sb_irq_frame_t *scheduler_reschedule(sb_irq_frame_t *current_frame);
sb_irq_frame_t *scheduler_preempt(sb_irq_frame_t *current_frame);
void scheduler_switch_to(sb_task_t *next);

#endif