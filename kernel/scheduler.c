#include "scheduler.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/interrupts.h"
#include "mm/pmm.h"

#define SB_BOOTSTRAP_TASK_ID 1u
#define SB_BOOTSTRAP_PRIORITY 128u
#define SB_SCHED_MAX_TASKS 64u
#define SB_INITIAL_RFLAGS 0x202u
#define SB_MAX_SLEEP_TICKS 0x7FFFFFFFFFFFFFFFull

/* Scheduler starts before the kernel has initialized SIMD/FPU state. Keep the
 * task table volatile so GCC cannot synthesize SSE vector stores for struct
 * initialization or updates. task_count is the high-water slot count; holes
 * left by reaping are reused before growing it. */
static volatile sb_task_t tasks[SB_SCHED_MAX_TASKS];
static uint32_t task_count;
static uint32_t current_index;
static uint64_t scheduler_tick_count;
static uint32_t preemption_validation_stage;
static uint32_t sleep_validation_stage;
static uint64_t sleep_validation_task_id;
static int cross_process_switch_logged;

extern char stack_top;
extern void sb_context_switch(sb_task_context_t *old_context,
                              const sb_task_context_t *new_context);

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

static uint64_t read_cr3(void) {
    uint64_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static void write_cr3(uint64_t value) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(value) : "memory");
}

static int tick_reached(uint64_t now, uint64_t deadline) {
    return (int64_t)(now - deadline) >= 0;
}

static void clear_task(volatile sb_task_t *task) {
    task->id = 0u;
    task->process_id = 0u;
    task->runtime_ticks = 0u;
    task->dispatch_count = 0u;
    task->wake_tick = 0u;
    task->exit_code = 0;
    task->priority = 0u;
    task->state = SB_TASK_UNUSED;
    task->context = (sb_task_context_t){0};
    task->irq_frame_rsp = 0u;
    task->address_space_cr3 = 0u;
    task->kernel_stack_base = 0u;
    task->kernel_stack_top = 0u;
    task->user_task = 0u;
}

static int task_index_of(const sb_task_t *task, uint32_t *index) {
    if (task == 0 || index == 0) return -1;
    for (uint32_t i = 0u; i < task_count; ++i) {
        if ((const sb_task_t *)(uintptr_t)&tasks[i] == task) {
            *index = i;
            return 0;
        }
    }
    return -1;
}

static sb_task_t *task_by_id(uint64_t id) {
    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state != SB_TASK_UNUSED && tasks[i].id == id) {
            return (sb_task_t *)(uintptr_t)&tasks[i];
        }
    }
    return 0;
}

static int task_id_exists(uint64_t id) {
    return task_by_id(id) != 0;
}

static int find_task_slot(uint32_t *index) {
    if (index == 0) return -1;
    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state == SB_TASK_UNUSED) {
            *index = i;
            return 0;
        }
    }
    if (task_count >= SB_SCHED_MAX_TASKS) return -1;
    *index = task_count;
    return 0;
}

static void commit_task_slot(uint32_t index) {
    if (index == task_count && task_count < SB_SCHED_MAX_TASKS) ++task_count;
}

static void trim_unused_tail(void) {
    while (task_count > 1u && tasks[task_count - 1u].state == SB_TASK_UNUSED) {
        --task_count;
    }
}

static void mark_task_ready(sb_task_t *task) {
    if (task == 0) return;
    task->state = SB_TASK_READY;
    task->wake_tick = 0u;
    if (sleep_validation_stage == 2u && task->id == sleep_validation_task_id) {
        sleep_validation_stage = 3u;
        sched_debug("Scheduler: sleeping user task woke\r\n");
    }
}

void scheduler_init(void) {
    sched_debug("[SCHED] init begin\r\n");
    for (uint32_t i = 0u; i < SB_SCHED_MAX_TASKS; ++i) clear_task(&tasks[i]);

    tasks[0].id = SB_BOOTSTRAP_TASK_ID;
    tasks[0].process_id = 0u;
    tasks[0].runtime_ticks = 0u;
    tasks[0].dispatch_count = 1u;
    tasks[0].wake_tick = 0u;
    tasks[0].priority = SB_BOOTSTRAP_PRIORITY;
    tasks[0].state = SB_TASK_RUNNING;
    tasks[0].address_space_cr3 = read_cr3();
    tasks[0].kernel_stack_top = (uint64_t)(uintptr_t)&stack_top;
    sched_debug("[SCHED] bootstrap task ready\r\n");

    task_count = 1u;
    current_index = 0u;
    scheduler_tick_count = 0u;
    preemption_validation_stage = 0u;
    sleep_validation_stage = 0u;
    sleep_validation_task_id = 0u;
    cross_process_switch_logged = 0;
    sched_debug("[SCHED] scalar state ready\r\n");
}

void scheduler_tick(void) {
    ++scheduler_tick_count;

    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state == SB_TASK_SLEEPING &&
            tick_reached(scheduler_tick_count, tasks[i].wake_tick)) {
            mark_task_ready((sb_task_t *)(uintptr_t)&tasks[i]);
        }
    }

    if (task_count == 0u || current_index >= task_count) return;
    if (tasks[current_index].state == SB_TASK_RUNNING) ++tasks[current_index].runtime_ticks;
}

uint64_t scheduler_ticks(void) { return scheduler_tick_count; }

sb_task_t *scheduler_current(void) {
    if (task_count == 0u || current_index >= task_count ||
        tasks[current_index].state == SB_TASK_UNUSED) return 0;
    return (sb_task_t *)(uintptr_t)&tasks[current_index];
}

int scheduler_add_kernel_task(uint64_t id, uint32_t priority) {
    if (id == 0u) return -1;
    if (task_id_exists(id)) return -2;

    uint32_t index;
    if (find_task_slot(&index) != 0) return -1;

    volatile sb_task_t *task = &tasks[index];
    clear_task(task);
    task->id = id;
    task->priority = priority;
    task->state = SB_TASK_READY;
    task->address_space_cr3 = read_cr3();
    commit_task_slot(index);
    return 0;
}

int scheduler_add_user_task(uint64_t id,
                            uint64_t process_id,
                            uint32_t priority,
                            uint64_t address_space_cr3,
                            uint64_t user_entry,
                            uint64_t user_stack_top) {
    if (id == 0u || process_id == 0u || address_space_cr3 == 0u ||
        user_entry == 0u || user_stack_top == 0u) return -1;
    if (task_id_exists(id)) return -2;

    uint32_t index;
    if (find_task_slot(&index) != 0) return -1;

    void *stack_page = pmm_alloc_page();
    if (stack_page == 0) return -3;

    const uint64_t kernel_stack_base = (uint64_t)(uintptr_t)stack_page;
    const uint64_t kernel_stack_top = kernel_stack_base + SB_PAGE_SIZE;
    sb_irq_frame_t *frame = (sb_irq_frame_t *)(uintptr_t)(kernel_stack_top - sizeof(sb_irq_frame_t));
    volatile uint64_t *words = (volatile uint64_t *)(uintptr_t)frame;
    for (uint32_t i = 0u; i < (uint32_t)(sizeof(*frame) / sizeof(uint64_t)); ++i) words[i] = 0u;

    frame->rip = user_entry;
    frame->cs = SB_USER_CODE_SELECTOR;
    frame->rflags = SB_INITIAL_RFLAGS;
    frame->rsp = user_stack_top;
    frame->ss = SB_USER_DATA_SELECTOR;

    volatile sb_task_t *task = &tasks[index];
    clear_task(task);
    task->id = id;
    task->process_id = process_id;
    task->priority = priority;
    task->state = SB_TASK_READY;
    task->irq_frame_rsp = (uint64_t)(uintptr_t)frame;
    task->address_space_cr3 = address_space_cr3;
    task->kernel_stack_base = kernel_stack_base;
    task->kernel_stack_top = kernel_stack_top;
    task->user_task = 1u;
    commit_task_slot(index);
    return 0;
}

sb_task_t *scheduler_pick_next(void) {
    if (task_count == 0u) return 0;
    for (uint32_t step = 1u; step <= task_count; ++step) {
        const uint32_t candidate = (current_index + step) % task_count;
        if (tasks[candidate].state == SB_TASK_READY || tasks[candidate].state == SB_TASK_RUNNING) {
            return (sb_task_t *)(uintptr_t)&tasks[candidate];
        }
    }
    return 0;
}

static sb_task_t *pick_preemptable_next(void) {
    if (task_count == 0u) return 0;
    for (uint32_t step = 1u; step <= task_count; ++step) {
        const uint32_t candidate = (current_index + step) % task_count;
        if ((tasks[candidate].state == SB_TASK_READY || tasks[candidate].state == SB_TASK_RUNNING) &&
            tasks[candidate].irq_frame_rsp != 0u) {
            return (sb_task_t *)(uintptr_t)&tasks[candidate];
        }
    }
    return 0;
}

int scheduler_block_current(void) {
    sb_task_t *current = scheduler_current();
    if (current == 0) return -1;
    if (current->user_task == 0u) return -2; /* bootstrap task is the current idle fallback */
    if (current->state != SB_TASK_RUNNING) return -3;
    current->state = SB_TASK_BLOCKED;
    current->wake_tick = 0u;
    return 0;
}

int scheduler_sleep_current(uint64_t delay_ticks) {
    sb_task_t *current = scheduler_current();
    if (current == 0) return -1;
    if (current->user_task == 0u) return -2; /* keep bootstrap runnable as the idle fallback */
    if (current->state != SB_TASK_RUNNING) return -3;
    if (delay_ticks > SB_MAX_SLEEP_TICKS) return -4;
    if (delay_ticks == 0u) delay_ticks = 1u;

    current->wake_tick = scheduler_tick_count + delay_ticks;
    current->state = SB_TASK_SLEEPING;
    if (sleep_validation_stage == 0u) {
        sleep_validation_stage = 1u;
        sleep_validation_task_id = current->id;
        sched_debug("Scheduler: user task entered SLEEPING\r\n");
    }
    return 0;
}

int scheduler_wake_task(uint64_t id) {
    sb_task_t *task = task_by_id(id);
    if (task == 0) return -1;
    if (task->state != SB_TASK_BLOCKED && task->state != SB_TASK_SLEEPING) return -2;
    mark_task_ready(task);
    return 0;
}

int scheduler_exit_current_process(int64_t exit_code) {
    sb_task_t *current = scheduler_current();
    if (current == 0 || current->user_task == 0u || current->process_id == 0u) return -1;
    if (current->state != SB_TASK_RUNNING) return -2;

    const uint64_t pid = current->process_id;
    int marked = 0;
    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state == SB_TASK_UNUSED || tasks[i].process_id != pid) continue;
        tasks[i].state = SB_TASK_EXITED;
        tasks[i].wake_tick = 0u;
        tasks[i].exit_code = exit_code;
        marked = 1;
    }
    return marked ? 0 : -3;
}

int scheduler_reap_process(uint64_t pid) {
    if (pid == 0u) return -1;
    sb_task_t *current = scheduler_current();
    if (current != 0 && current->process_id == pid) return -2;

    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state == SB_TASK_UNUSED || tasks[i].process_id != pid) continue;
        if (tasks[i].state != SB_TASK_EXITED) return -3;
    }

    int reaped = 0;
    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state != SB_TASK_EXITED || tasks[i].process_id != pid) continue;
        if (tasks[i].kernel_stack_base != 0u) {
            pmm_free_page((void *)(uintptr_t)tasks[i].kernel_stack_base);
        }
        clear_task(&tasks[i]);
        ++reaped;
    }
    trim_unused_tail();
    return reaped;
}

static void report_preemption_transition(const sb_task_t *current, const sb_task_t *next) {
    if (current == 0 || next == 0) return;

    if (preemption_validation_stage == 0u && current->user_task == 0u && next->user_task != 0u) {
        preemption_validation_stage = 1u;
        sched_debug("Scheduler: timer preemption switched to user task\r\n");
        sched_debug("Userspace: entering ring3\r\n");
        return;
    }

    if (preemption_validation_stage == 1u && current->user_task != 0u && next->user_task == 0u) {
        preemption_validation_stage = 2u;
        sched_debug("Scheduler: user task preempted back to kernel\r\n");
        return;
    }

    if (preemption_validation_stage == 2u && current->user_task == 0u && next->user_task != 0u) {
        preemption_validation_stage = 3u;
        sched_debug("Scheduler: saved user IRQ frame selected for resume\r\n");
    }
}

static void report_sleep_switch(const sb_task_t *current, const sb_task_t *next, int timer_driven) {
    if (current == 0 || next == 0) return;

    if (!timer_driven && sleep_validation_stage == 1u &&
        current->id == sleep_validation_task_id && current->state == SB_TASK_SLEEPING) {
        sleep_validation_stage = 2u;
        sched_debug("Scheduler: sleeping user task descheduled\r\n");
        return;
    }

    if (sleep_validation_stage == 3u && next->id == sleep_validation_task_id) {
        sleep_validation_stage = 4u;
        sched_debug("Scheduler: woke user task selected for resume\r\n");
    }
}

static void report_cross_process_switch(const sb_task_t *current, const sb_task_t *next) {
    if (cross_process_switch_logged || current == 0 || next == 0) return;
    if (current->user_task == 0u || next->user_task == 0u) return;
    if (current->process_id == 0u || next->process_id == 0u ||
        current->process_id == next->process_id) return;
    if (current->address_space_cr3 == 0u || next->address_space_cr3 == 0u ||
        current->address_space_cr3 == next->address_space_cr3) return;

    cross_process_switch_logged = 1;
    sched_debug("Scheduler: cross-process CR3 switch verified\r\n");
}

static sb_irq_frame_t *scheduler_switch_frame(sb_irq_frame_t *current_frame, int timer_driven) {
    if (current_frame == 0 || task_count == 0u) return current_frame;

    sb_task_t *current = scheduler_current();
    if (current == 0) return current_frame;
    current->irq_frame_rsp = (uint64_t)(uintptr_t)current_frame;

    sb_task_t *next = pick_preemptable_next();
    if (next == 0 || next == current || next->irq_frame_rsp == 0u) return current_frame;

    uint32_t next_index;
    if (task_index_of(next, &next_index) != 0) return current_frame;

    if (current->state == SB_TASK_RUNNING) current->state = SB_TASK_READY;
    next->state = SB_TASK_RUNNING;
    ++next->dispatch_count;
    current_index = next_index;

    report_cross_process_switch(current, next);

    if (next->address_space_cr3 != 0u && next->address_space_cr3 != read_cr3()) {
        write_cr3(next->address_space_cr3);
    }
    if (next->kernel_stack_top != 0u) gdt_set_kernel_stack(next->kernel_stack_top);

    if (timer_driven) report_preemption_transition(current, next);
    report_sleep_switch(current, next, timer_driven);
    return (sb_irq_frame_t *)(uintptr_t)next->irq_frame_rsp;
}

sb_irq_frame_t *scheduler_reschedule(sb_irq_frame_t *current_frame) {
    return scheduler_switch_frame(current_frame, 0);
}

sb_irq_frame_t *scheduler_preempt(sb_irq_frame_t *current_frame) {
    return scheduler_switch_frame(current_frame, 1);
}

void scheduler_switch_to(sb_task_t *next) {
    sb_task_t *current = scheduler_current();
    if (current == 0 || next == 0 || current == next) return;
    if (next->context.rsp == 0u || next->context.rip == 0u) return;

    uint32_t next_index;
    if (task_index_of(next, &next_index) != 0) return;

    interrupts_disable();
    if (current->state == SB_TASK_RUNNING) current->state = SB_TASK_READY;
    next->state = SB_TASK_RUNNING;
    ++next->dispatch_count;
    current_index = next_index;
    sb_context_switch((sb_task_context_t *)(uintptr_t)&current->context,
                      (const sb_task_context_t *)(uintptr_t)&next->context);
    interrupts_enable();
}

uint32_t scheduler_task_count(void) {
    uint32_t live = 0u;
    for (uint32_t i = 0u; i < task_count; ++i) {
        if (tasks[i].state != SB_TASK_UNUSED) ++live;
    }
    return live;
}
