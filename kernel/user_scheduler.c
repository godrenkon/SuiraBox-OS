#include "user_scheduler.h"
#include "arch/x86_64/gdt.h"

#define SB_USER_CS_RPL_MASK 0x3u

typedef struct {
    sb_process_t *process;
    sb_thread_t *thread;
} sb_user_sched_slot_t;

static sb_user_sched_slot_t slots[SB_MAX_USER_SCHED_THREADS];
static uint32_t slot_count;
static uint32_t current_index;
static uint64_t quantum_ticks;

static int slot_matches(const sb_user_sched_slot_t *slot,
                        const sb_process_t *process,
                        const sb_thread_t *thread) {
    return slot != 0 && slot->process == process && slot->thread == thread;
}

static int thread_runnable(const sb_thread_t *thread) {
    return thread != 0 && thread->user_context != 0 &&
           thread->kernel_resume_stack_pointer != 0u &&
           (thread->state == SB_PROCESS_CREATED || thread->state == SB_PROCESS_RUNNING);
}

static int process_runnable(const sb_process_t *process) {
    return process != 0 &&
           (process->state == SB_PROCESS_CREATED || process->state == SB_PROCESS_RUNNING);
}

static void remove_slot_at(uint32_t index) {
    if (index >= slot_count) return;
    for (uint32_t i = index + 1u; i < slot_count; ++i) slots[i - 1u] = slots[i];
    slots[slot_count - 1u].process = 0;
    slots[slot_count - 1u].thread = 0;
    --slot_count;
    if (slot_count == 0u) {
        current_index = 0u;
        quantum_ticks = 0u;
    } else if (current_index >= slot_count) {
        current_index = 0u;
    } else if (index < current_index) {
        --current_index;
    }
}

static void prune_dead_slots(void) {
    uint32_t i = 0u;
    while (i < slot_count) {
        if (!thread_runnable(slots[i].thread) || !process_runnable(slots[i].process))
            remove_slot_at(i);
        else
            ++i;
    }
}

void user_scheduler_init(void) {
    for (uint32_t i = 0u; i < SB_MAX_USER_SCHED_THREADS; ++i) {
        slots[i].process = 0;
        slots[i].thread = 0;
    }
    slot_count = 0u;
    current_index = 0u;
    quantum_ticks = 0u;
}

int user_scheduler_add(sb_process_t *process, sb_thread_t *thread) {
    prune_dead_slots();
    if (!process_runnable(process) || !thread_runnable(thread)) return -1;
    for (uint32_t i = 0u; i < slot_count; ++i) {
        if (slot_matches(&slots[i], process, thread)) return -2;
    }
    if (slot_count >= SB_MAX_USER_SCHED_THREADS) return -3;
    slots[slot_count].process = process;
    slots[slot_count].thread = thread;
    ++slot_count;
    return 0;
}

int user_scheduler_remove(sb_process_t *process, sb_thread_t *thread) {
    for (uint32_t i = 0u; i < slot_count; ++i) {
        if (slot_matches(&slots[i], process, thread)) {
            remove_slot_at(i);
            return 0;
        }
    }
    return -1;
}

int user_scheduler_set_current(sb_process_t *process, sb_thread_t *thread) {
    prune_dead_slots();
    if (!thread_runnable(thread) || !process_runnable(process)) return -1;
    for (uint32_t i = 0u; i < slot_count; ++i) {
        if (slot_matches(&slots[i], process, thread)) {
            current_index = i;
            quantum_ticks = 0u;
            return 0;
        }
    }
    return -1;
}

sb_thread_t *user_scheduler_current_thread(void) {
    prune_dead_slots();
    if (slot_count == 0u || current_index >= slot_count) return 0;
    return slots[current_index].thread;
}

sb_process_t *user_scheduler_current_process(void) {
    prune_dead_slots();
    if (slot_count == 0u || current_index >= slot_count) return 0;
    return slots[current_index].process;
}

uint32_t user_scheduler_count(void) {
    prune_dead_slots();
    return slot_count;
}

static int frame_is_user_mode(const sb_timer_saved_gpr_t *gpr,
                              sb_x86_64_user_iret_frame_t **iret_out) {
    if (gpr == 0 || iret_out == 0) return 0;
    const uint64_t *hardware_frame = (const uint64_t *)((const uint8_t *)gpr + sizeof(*gpr));
    const uint64_t cs = hardware_frame[1];
    if ((cs & SB_USER_CS_RPL_MASK) != SB_USER_CS_RPL_MASK) return 0;
    *iret_out = (sb_x86_64_user_iret_frame_t *)(uintptr_t)hardware_frame;
    return 1;
}

uintptr_t user_scheduler_timer_dispatch(sb_timer_saved_gpr_t *gpr) {
    if (gpr == 0) return 0u;

    sb_x86_64_user_iret_frame_t *iret = 0;
    if (!frame_is_user_mode(gpr, &iret)) return (uintptr_t)gpr;

    prune_dead_slots();
    if (slot_count == 0u || current_index >= slot_count) return (uintptr_t)gpr;

    sb_thread_t *current_thread = slots[current_index].thread;
    sb_process_t *current_process = slots[current_index].process;
    if (!thread_runnable(current_thread) || !process_runnable(current_process)) return (uintptr_t)gpr;
    if (sb_user_context_from_timer_frame(current_thread->user_context, gpr, iret) != 0)
        return (uintptr_t)gpr;

    ++quantum_ticks;
    if (slot_count <= 1u || (quantum_ticks % SB_USER_SCHED_QUANTUM_TICKS) != 0u)
        return (uintptr_t)gpr;

    for (uint32_t step = 1u; step <= slot_count; ++step) {
        const uint32_t candidate = (current_index + step) % slot_count;
        sb_process_t *next_process = slots[candidate].process;
        sb_thread_t *next_thread = slots[candidate].thread;
        if (!thread_runnable(next_thread) || !process_runnable(next_process)) continue;
        if (next_thread == current_thread) continue;
        if (process_prepare_user_resume_frame(next_thread) != 0) continue;
        if (process_activate(next_process) != 0) continue;
        if (gdt_try_set_kernel_stack(next_thread->kernel_stack_top) != 0) {
            (void)process_activate(current_process);
            (void)gdt_try_set_kernel_stack(current_thread->kernel_stack_top);
            continue;
        }

        current_thread->state = SB_PROCESS_CREATED;
        next_thread->state = SB_PROCESS_RUNNING;
        current_process->state = SB_PROCESS_RUNNING;
        next_process->state = SB_PROCESS_RUNNING;
        current_index = candidate;
        quantum_ticks = 0u;
        return (uintptr_t)next_thread->kernel_resume_stack_pointer;
    }

    return (uintptr_t)gpr;
}
