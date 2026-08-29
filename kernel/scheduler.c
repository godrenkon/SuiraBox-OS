#include "scheduler.h"
#include "timer.h"
#include "arch/x86_64/interrupts.h"

#define SB_BOOTSTRAP_TASK_ID 1u
#define SB_BOOTSTRAP_PRIORITY 128u
#define SB_SCHED_MAX_TASKS 64u

/* Scheduler starts before the kernel has initialized SIMD/FPU state. Keep the
 * task table volatile so GCC cannot synthesize SSE vector stores for struct
 * initialization or updates. */
static volatile sb_task_t tasks[SB_SCHED_MAX_TASKS];
static uint32_t task_count;
static uint32_t current_index;
static uint64_t scheduler_tick_count;

extern void sb_context_switch(sb_task_context_t *old_context,
                              const sb_task_context_t *new_context);

#ifdef SB_KERNEL_DEBUG
static void sched_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void sched_debug(const char *s) {
    while (*s) sched_debug_char(*s++);
}
#define SCHED_DEBUG(message) sched_debug(message)
#else
#define SCHED_DEBUG(message) do { (void)sizeof(message); } while (0)
#endif

static void clear_task(volatile sb_task_t *task) {
    task->id = 0u;
    task->runtime_ticks = 0u;
    task->priority = 0u;
    task->state = SB_TASK_UNUSED;
    task->context = (sb_task_context_t){0};
    task->kernel_stack_base = 0u;
    task->kernel_stack_top = 0u;
}

void scheduler_init(void) {
    SCHED_DEBUG("[SCHED] init begin\r\n");
    for (uint32_t i = 0u; i < SB_SCHED_MAX_TASKS; ++i) clear_task(&tasks[i]);

    tasks[0].id = SB_BOOTSTRAP_TASK_ID;
    tasks[0].runtime_ticks = 0u;
    tasks[0].priority = SB_BOOTSTRAP_PRIORITY;
    tasks[0].state = SB_TASK_RUNNING;
    SCHED_DEBUG("[SCHED] bootstrap task ready\r\n");

    task_count = 1u;
    current_index = 0u;
    scheduler_tick_count = 0u;
    SCHED_DEBUG("[SCHED] scalar state ready\r\n");
}

void scheduler_tick(void) {
    ++scheduler_tick_count;
    if (task_count == 0u) return;
    if (tasks[current_index].state == SB_TASK_RUNNING) ++tasks[current_index].runtime_ticks;
}

uint64_t scheduler_ticks(void) { return scheduler_tick_count; }

sb_task_t *scheduler_current(void) {
    if (task_count == 0u) return 0;
    return (sb_task_t *)(uintptr_t)&tasks[current_index];
}

int scheduler_add_kernel_task(uint64_t id, uint32_t priority) {
    if (task_count >= SB_SCHED_MAX_TASKS || id == 0u) return -1;
    for (uint32_t i = 0u; i < task_count; ++i)
        if (tasks[i].state != SB_TASK_UNUSED && tasks[i].id == id) return -2;

    tasks[task_count].id = id;
    tasks[task_count].runtime_ticks = 0u;
    tasks[task_count].priority = priority;
    tasks[task_count].state = SB_TASK_READY;
    tasks[task_count].context = (sb_task_context_t){0};
    tasks[task_count].kernel_stack_base = 0u;
    tasks[task_count].kernel_stack_top = 0u;
    ++task_count;
    return 0;
}

sb_task_t *scheduler_pick_next(void) {
    if (task_count == 0u) return 0;
    for (uint32_t step = 1u; step <= task_count; ++step) {
        const uint32_t candidate = (current_index + step) % task_count;
        if (tasks[candidate].state == SB_TASK_READY || tasks[candidate].state == SB_TASK_RUNNING) {
            tasks[current_index].state = SB_TASK_READY;
            current_index = candidate;
            tasks[current_index].state = SB_TASK_RUNNING;
            return (sb_task_t *)(uintptr_t)&tasks[current_index];
        }
    }
    return (sb_task_t *)(uintptr_t)&tasks[current_index];
}

void scheduler_switch_to(sb_task_t *next) {
    sb_task_t *current = scheduler_current();
    if (current == 0 || next == 0 || current == next) return;
    if (current->context.rsp == 0u || current->context.rip == 0u ||
        next->context.rsp == 0u || next->context.rip == 0u) return;

    interrupts_disable();
    current->state = SB_TASK_READY;
    next->state = SB_TASK_RUNNING;
    current_index = (uint32_t)(next - (sb_task_t *)(uintptr_t)&tasks[0]);
    sb_context_switch((sb_task_context_t *)(uintptr_t)&current->context,
                      (const sb_task_context_t *)(uintptr_t)&next->context);
    interrupts_enable();
}

uint32_t scheduler_task_count(void) { return task_count; }
