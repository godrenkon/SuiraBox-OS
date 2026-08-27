#include "scheduler.h"
#include "timer.h"

#define SB_BOOTSTRAP_TASK_ID 1u
#define SB_BOOTSTRAP_PRIORITY 128u
#define SB_SCHED_MAX_TASKS 64u

static sb_task_t tasks[SB_SCHED_MAX_TASKS];
static uint32_t task_count;
static uint32_t current_index;
static uint64_t scheduler_tick_count;

void scheduler_init(void) {
    for (uint32_t i = 0; i < SB_SCHED_MAX_TASKS; ++i) {
        tasks[i] = (sb_task_t){0};
        tasks[i].state = SB_TASK_UNUSED;
    }

    tasks[0].id = SB_BOOTSTRAP_TASK_ID;
    tasks[0].runtime_ticks = 0;
    tasks[0].priority = SB_BOOTSTRAP_PRIORITY;
    tasks[0].state = SB_TASK_RUNNING;

    task_count = 1u;
    current_index = 0u;
    scheduler_tick_count = 0;
}

void scheduler_tick(void) {
    ++scheduler_tick_count;

    if (task_count == 0u) {
        return;
    }

    if (tasks[current_index].state == SB_TASK_RUNNING) {
        ++tasks[current_index].runtime_ticks;
    }
}

uint64_t scheduler_ticks(void) {
    return scheduler_tick_count;
}

sb_task_t *scheduler_current(void) {
    if (task_count == 0u) {
        return 0;
    }
    return &tasks[current_index];
}

int scheduler_add_kernel_task(uint64_t id, uint32_t priority) {
    if (task_count >= SB_SCHED_MAX_TASKS || id == 0u) {
        return -1;
    }

    tasks[task_count].id = id;
    tasks[task_count].runtime_ticks = 0;
    tasks[task_count].priority = priority;
    tasks[task_count].state = SB_TASK_READY;
    ++task_count;
    return 0;
}

sb_task_t *scheduler_pick_next(void) {
    if (task_count == 0u) {
        return 0;
    }

    for (uint32_t step = 1u; step <= task_count; ++step) {
        const uint32_t candidate = (current_index + step) % task_count;
        if (tasks[candidate].state == SB_TASK_READY ||
            tasks[candidate].state == SB_TASK_RUNNING) {
            tasks[current_index].state = SB_TASK_READY;
            current_index = candidate;
            tasks[current_index].state = SB_TASK_RUNNING;
            return &tasks[current_index];
        }
    }

    return &tasks[current_index];
}

uint32_t scheduler_task_count(void) {
    return task_count;
}
