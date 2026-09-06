#include "process.h"
#include "scheduler.h"

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;

static void clear_process(sb_process_t *process) {
    if (process == 0) return;
    *process = (sb_process_t){0};
    process->state = SB_PROCESS_UNUSED;
}

void process_init(void) {
    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        clear_process(&processes[i]);
    }
    process_count_value = 0;
}

sb_process_t *process_get(uint64_t pid) {
    if (pid == 0u) return 0;
    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return 0;
}

sb_process_t *process_create(uint64_t pid) {
    if (pid == 0u || process_count_value >= SB_MAX_PROCESSES || process_get(pid) != 0) {
        return 0;
    }

    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED) continue;

        clear_process(&processes[i]);
        processes[i].pid = pid;
        processes[i].state = SB_PROCESS_CREATED;
        if (address_space_create(&processes[i].address_space) != 0) {
            clear_process(&processes[i]);
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

    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        if (process->threads[i].tid == tid) return 0;
    }

    sb_thread_t *thread = &process->threads[process->thread_count++];
    *thread = (sb_thread_t){0};
    thread->tid = tid;
    thread->priority = priority;
    thread->state = SB_PROCESS_CREATED;
    return thread;
}

uint32_t process_count(void) {
    return process_count_value;
}

int process_activate(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) {
        return -1;
    }
    return address_space_activate(&process->address_space);
}

int process_mark_exited(uint64_t pid, int64_t exit_code) {
    sb_process_t *process = process_get(pid);
    if (process == 0 || process->state == SB_PROCESS_EXITED) return -1;

    process->exit_code = exit_code;
    process->state = SB_PROCESS_EXITED;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        process->threads[i].state = SB_PROCESS_EXITED;
    }
    return 0;
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    address_space_destroy(&process->address_space);
    clear_process(process);
    if (process_count_value > 0u) --process_count_value;
}

uint32_t process_reap_exited(void) {
    uint32_t reaped = 0u;

    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        sb_process_t *process = &processes[i];
        if (process->state != SB_PROCESS_EXITED) continue;

        const int task_result = scheduler_reap_process(process->pid);
        if (task_result < 0) continue;

        process_destroy(process);
        ++reaped;
    }

    return reaped;
}
