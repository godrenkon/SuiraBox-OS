#include "process.h"

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;

void process_init(void) {
    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        processes[i] = (sb_process_t){0};
        processes[i].state = SB_PROCESS_UNUSED;
    }
    process_count_value = 0;
}

sb_process_t *process_create(uint64_t pid) {
    if (pid == 0u || process_count_value >= SB_MAX_PROCESSES) {
        return 0;
    }

    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED) {
            continue;
        }
        processes[i].pid = pid;
        processes[i].state = SB_PROCESS_CREATED;
        processes[i].thread_count = 0;
        if (address_space_create(&processes[i].address_space) != 0) {
            processes[i].state = SB_PROCESS_UNUSED;
            processes[i].pid = 0;
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

    sb_thread_t *thread = &process->threads[process->thread_count++];
    *thread = (sb_thread_t){0};
    thread->tid = tid;
    thread->priority = priority;
    thread->state = SB_PROCESS_CREATED;
    return thread;
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

uint32_t process_count(void) {
    return process_count_value;
}

int process_activate(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) {
        return -1;
    }
    return address_space_activate(&process->address_space);
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    address_space_destroy(&process->address_space);
    process->state = SB_PROCESS_UNUSED;
    process->pid = 0;
    process->thread_count = 0;
    if (process_count_value > 0u) --process_count_value;
}
