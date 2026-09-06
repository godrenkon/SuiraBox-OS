#include "process.h"
#include "scheduler.h"

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;

static void clear_process(sb_process_t *process) {
    if (process == 0) return;
    *process = (sb_process_t){0};
    process->state = SB_PROCESS_UNUSED;
}

static void release_process_resources(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;

    /* Handle callbacks run before the address space disappears so future
     * resource types can release process-owned backing objects in a stable
     * teardown phase. Scheduler-owned task stacks have already been reaped in
     * the normal EXIT path before this helper is reached. */
    (void)sb_handle_close_all(&process->handles);
    address_space_destroy(&process->address_space);
    process->entry_point = 0u;
    process->user_stack_top = 0u;
}

static void collect_process_slot(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    clear_process(process);
    if (process_count_value > 0u) --process_count_value;
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
        sb_handle_table_init(&processes[i].handles);
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
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED ||
        process->state == SB_PROCESS_ZOMBIE) {
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
    if (process == 0 || process->state == SB_PROCESS_UNUSED ||
        process->state == SB_PROCESS_EXITED || process->state == SB_PROCESS_ZOMBIE) {
        return -1;
    }
    return address_space_activate(&process->address_space);
}

int process_mark_exited(uint64_t pid, int64_t exit_code) {
    sb_process_t *process = process_get(pid);
    if (process == 0 || process->state == SB_PROCESS_EXITED ||
        process->state == SB_PROCESS_ZOMBIE) return -1;

    process->exit_code = exit_code;
    process->state = SB_PROCESS_EXITED;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        process->threads[i].state = SB_PROCESS_EXITED;
    }
    return 0;
}

int process_wait_child(uint64_t parent_pid,
                       uint64_t child_pid,
                       uint64_t waiter_tid,
                       int64_t *exit_code) {
    if (parent_pid == 0u || child_pid == 0u || waiter_tid == 0u || exit_code == 0) return -1;

    sb_process_t *parent = process_get(parent_pid);
    sb_process_t *child = process_get(child_pid);
    if (parent == 0 || child == 0 || child->parent_pid != parent_pid) return -2;
    if (parent->state == SB_PROCESS_EXITED || parent->state == SB_PROCESS_ZOMBIE) return -3;

    if (child->state == SB_PROCESS_ZOMBIE) {
        *exit_code = child->exit_code;
        process_destroy(child);
        return 0;
    }

    if (child->waiter_tid != 0u && child->waiter_tid != waiter_tid) return -4;
    child->waiter_tid = waiter_tid;
    return 1;
}

int process_cancel_wait(uint64_t child_pid, uint64_t waiter_tid) {
    sb_process_t *child = process_get(child_pid);
    if (child == 0 || waiter_tid == 0u || child->waiter_tid != waiter_tid) return -1;
    child->waiter_tid = 0u;
    return 0;
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    release_process_resources(process);
    collect_process_slot(process);
}

uint32_t process_reap_exited(void) {
    uint32_t reaped = 0u;

    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        sb_process_t *process = &processes[i];
        if (process->state != SB_PROCESS_EXITED) continue;

        const int task_result = scheduler_reap_process(process->pid);
        if (task_result < 0) continue;

        /* Task stacks are gone and this CR3 is no longer executing. Close all
         * process-local resource handles, then release the user address space
         * before making exit status visible to a waiting parent. */
        release_process_resources(process);
        ++reaped;

        if (process->waiter_tid != 0u) {
            const uint64_t waiter_tid = process->waiter_tid;
            const uint64_t result = (uint64_t)process->exit_code;
            if (scheduler_wake_task_with_result(waiter_tid, result) == 0) {
                collect_process_slot(process);
                continue;
            }
            process->waiter_tid = 0u;
        }

        /* No active waiter: preserve only process metadata/exit status. */
        process->state = SB_PROCESS_ZOMBIE;
    }

    return reaped;
}
