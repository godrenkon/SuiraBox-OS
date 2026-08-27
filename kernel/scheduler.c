#include "scheduler.h"
#include "timer.h"

#define SB_BOOTSTRAP_TASK_ID 1u
#define SB_BOOTSTRAP_PRIORITY 128u

static sb_task_t bootstrap_task;
static uint64_t scheduler_tick_count;

void scheduler_init(void) {
    bootstrap_task.id = SB_BOOTSTRAP_TASK_ID;
    bootstrap_task.runtime_ticks = 0;
    bootstrap_task.priority = SB_BOOTSTRAP_PRIORITY;
    bootstrap_task.state = SB_TASK_RUNNING;
    scheduler_tick_count = 0;
}

void scheduler_tick(void) {
    ++scheduler_tick_count;
    if (bootstrap_task.state == SB_TASK_RUNNING) {
        ++bootstrap_task.runtime_ticks;
    }
}

uint64_t scheduler_ticks(void) {
    return scheduler_tick_count;
}

sb_task_t *scheduler_current(void) {
    return &bootstrap_task;
}
