#include "syscall_thread.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "process_exec.h"
#include <suirabox/syscall_abi.h>

#define SB_THREAD_DEFAULT_PRIORITY 128u

_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER == SB_SYS_THREAD_CREATE,
               "public syscall max-number table is stale");

static int thread_create_logged;

static void thread_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void thread_debug(const char *text) {
    while (*text) thread_debug_char(*text++);
}

sb_irq_frame_t *sb_syscall_dispatch_thread(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    if (frame->rax != SB_SYS_THREAD_CREATE) {
        frame->rax = (uint64_t)SB_SYS_ERROR_INVALID;
        return frame;
    }

    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) {
        frame->rax = (uint64_t)SB_SYS_ERROR_INVALID;
        return frame;
    }

    sb_process_t *process = process_get(task->process_id);
    if (process == 0) {
        frame->rax = (uint64_t)SB_SYS_ERROR_INVALID;
        return frame;
    }

    uint64_t tid = 0u;
    const int result = process_spawn_user_thread(process,
                                                 frame->rdi,
                                                 SB_THREAD_DEFAULT_PRIORITY,
                                                 &tid);
    if (result != 0 || tid == 0u) {
        frame->rax = result == -1 || result == -2
            ? (uint64_t)SB_SYS_ERROR_INVALID
            : (uint64_t)SB_SYS_ERROR_LIMIT;
        return frame;
    }

    frame->rax = tid;
    if (!thread_create_logged) {
        thread_create_logged = 1;
        thread_debug("Thread: THREAD_CREATE syscall registered user thread\r\n");
    }
    return frame;
}
