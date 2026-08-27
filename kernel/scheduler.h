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

typedef struct {
    uint64_t id;
    uint64_t runtime_ticks;
    uint32_t priority;
    sb_task_state_t state;
} sb_task_t;

void scheduler_init(void);
void scheduler_tick(void);
uint64_t scheduler_ticks(void);
sb_task_t *scheduler_current(void);

#endif
