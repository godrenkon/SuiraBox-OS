#include "process.h"
#include "mm/pmm.h"

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;

static int process_pid_in_use(uint64_t pid) {
    if (pid == 0u) return 1;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) return 1;
    }
    return 0;
}

static void clear_kernel_stack(uint8_t *stack) {
    for (uint32_t i = 0u; i < SB_USER_KERNEL_STACK_SIZE; ++i) stack[i] = 0u;
}

void process_init(void) {
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        processes[i] = (sb_process_t){0};
        processes[i].state = SB_PROCESS_UNUSED;
    }
    process_count_value = 0u;
}

sb_process_t *process_create(uint64_t pid) {
    if (process_count_value >= SB_MAX_PROCESSES || process_pid_in_use(pid)) return 0;

    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED) continue;

        processes[i] = (sb_process_t){0};
        processes[i].pid = pid;
        processes[i].state = SB_PROCESS_CREATED;
        if (address_space_create(&processes[i].address_space) != 0) {
            processes[i].state = SB_PROCESS_UNUSED;
            processes[i].pid = 0u;
            return 0;
        }

        ++process_count_value;
        return &processes[i];
    }

    return 0;
}

sb_thread_t *process_create_thread(sb_process_t *process, uint64_t tid, uint32_t priority) {
    if (process == 0 || tid == 0u || process->thread_count >= SB_MAX_THREADS_PER_PROCESS ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) {
        return 0;
    }

    for (uint32_t i = 0u; i < SB_MAX_THREADS_PER_PROCESS; ++i) {
        if (process->threads[i].tid == tid) return 0;
    }

    void *stack = pmm_alloc_page();
    if (stack == 0) return 0;
    clear_kernel_stack((uint8_t *)stack);

    sb_thread_t *thread = &process->threads[process->thread_count];
    *thread = (sb_thread_t){0};
    thread->tid = tid;
    thread->priority = priority;
    thread->state = SB_PROCESS_CREATED;
    thread->user_context = 0;
    thread->kernel_stack_base = (uint64_t)(uintptr_t)stack;
    thread->kernel_stack_top = thread->kernel_stack_base + SB_USER_KERNEL_STACK_SIZE;
    ++process->thread_count;
    return thread;
}

int process_prepare_thread_context(sb_thread_t *thread,
                                   sb_user_context_t *context,
                                   uint64_t entry_point,
                                   uint64_t user_stack_top) {
    if (thread == 0 || context == 0 || thread->tid == 0u ||
        thread->state == SB_PROCESS_UNUSED || thread->state == SB_PROCESS_EXITED ||
        thread->kernel_stack_base == 0u || thread->kernel_stack_top == 0u) return -1;
    if (sb_user_context_init(context, entry_point, user_stack_top) != 0) return -1;
    thread->user_context = context;
    return 0;
}

sb_process_t *process_get(uint64_t pid) {
    if (pid == 0u) return 0;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return 0;
}

uint32_t process_count(void) {
    return process_count_value;
}

int process_activate(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) {
        return -1;
    }
    if (process->address_space.pml4_physical == 0u) return -1;
    return address_space_activate(&process->address_space);
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        if (process->threads[i].kernel_stack_base != 0u) {
            pmm_free_page((void *)(uintptr_t)process->threads[i].kernel_stack_base);
            process->threads[i].kernel_stack_base = 0u;
            process->threads[i].kernel_stack_top = 0u;
        }
        process->threads[i].user_context = 0;
    }
    address_space_destroy(&process->address_space);
    *process = (sb_process_t){0};
    process->state = SB_PROCESS_UNUSED;
    if (process_count_value > 0u) --process_count_value;
}
