#include "syscall_shared_memory.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "handle.h"
#include "shared_memory.h"
#include <suirabox/syscall_abi.h>

_Static_assert(SB_HANDLE_TYPE_SHARED_MEMORY == SB_HANDLE_ABI_TYPE_SHARED_MEMORY,
               "kernel/public shared-memory handle type mismatch");
_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER >= SB_SYS_SHARED_MEMORY_UNMAP,
               "public syscall max-number table is stale");

typedef unsigned char sb_bool_t;

static int shared_create_logged;
static int shared_rw_map_logged;
static int shared_ro_map_logged;
static int shared_unmap_logged;

static void shared_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void shared_debug(const char *text) { while (*text) shared_debug_char(*text++); }
static uint64_t shared_error(int64_t code) { return (uint64_t)code; }

static sb_process_t *shared_current_process(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static uint64_t shared_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_STALE: return shared_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS: return shared_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_NO_SPACE: return shared_error(SB_SYS_ERROR_LIMIT);
        default: return shared_error(SB_SYS_ERROR_INVALID);
    }
}

static void shared_handle_close(void *object) {
    sb_shared_memory_release((sb_shared_memory_t *)object);
}

static sb_irq_frame_t *shared_create(sb_irq_frame_t *frame) {
    sb_process_t *process = shared_current_process();
    if (process == 0 || frame->rdi == 0u) {
        frame->rax = shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (frame->rdi > SB_SYS_SHARED_MEMORY_MAX) {
        frame->rax = shared_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    sb_shared_memory_t *memory = 0;
    if (sb_shared_memory_create(frame->rdi, &memory) != 0 || memory == 0) {
        frame->rax = shared_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int result = sb_handle_allocate(&process->handles,
                                          SB_HANDLE_TYPE_SHARED_MEMORY,
                                          SB_HANDLE_RIGHT_READ |
                                              SB_HANDLE_RIGHT_WRITE |
                                              SB_HANDLE_RIGHT_QUERY,
                                          memory,
                                          shared_handle_close,
                                          &handle);
    if (result != SB_HANDLE_OK) {
        sb_shared_memory_release(memory);
        frame->rax = shared_handle_error(result);
        return frame;
    }

    frame->rax = handle;
    if (!shared_create_logged) {
        shared_create_logged = 1;
        shared_debug("SharedMemory: userspace object handle created\r\n");
    }
    return frame;
}

static sb_irq_frame_t *shared_map(sb_irq_frame_t *frame) {
    sb_process_t *process = shared_current_process();
    if (process == 0 || frame->rsi == 0u) {
        frame->rax = shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t access = frame->rdx;
    if (access != SB_SHARED_MEMORY_ACCESS_READ &&
        access != (SB_SHARED_MEMORY_ACCESS_READ | SB_SHARED_MEMORY_ACCESS_WRITE)) {
        frame->rax = shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t rights = SB_HANDLE_RIGHT_READ |
        ((access & SB_SHARED_MEMORY_ACCESS_WRITE) != 0u ? SB_HANDLE_RIGHT_WRITE : 0u);
    sb_shared_memory_t *memory = 0;
    const int lookup = sb_handle_lookup(&process->handles,
                                        (sb_handle_t)frame->rdi,
                                        SB_HANDLE_TYPE_SHARED_MEMORY,
                                        rights,
                                        (void **)&memory);
    if (lookup != SB_HANDLE_OK || memory == 0) {
        frame->rax = shared_handle_error(lookup);
        return frame;
    }

    const sb_bool_t writable =
        (access & SB_SHARED_MEMORY_ACCESS_WRITE) != 0u ? 1u : 0u;
    const int map_result = process_map_shared_memory(process,
                                                     memory,
                                                     frame->rsi,
                                                     writable != 0u);
    if (map_result != 0) {
        frame->rax = map_result == -4
            ? shared_error(SB_SYS_ERROR_LIMIT)
            : shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    frame->rax = frame->rsi;
    if (writable != 0u) {
        if (!shared_rw_map_logged) {
            shared_rw_map_logged = 1;
            shared_debug("SharedMemory: writable mapping installed\r\n");
        }
    } else if (!shared_ro_map_logged) {
        shared_ro_map_logged = 1;
        shared_debug("SharedMemory: read-only alias installed\r\n");
    }
    return frame;
}

static sb_irq_frame_t *shared_unmap(sb_irq_frame_t *frame) {
    sb_process_t *process = shared_current_process();
    if (process == 0 || frame->rdi == 0u) {
        frame->rax = shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const int result = process_unmap_shared_memory(process, frame->rdi);
    if (result != 0) {
        frame->rax = result == -2
            ? shared_error(SB_SYS_ERROR_NOT_FOUND)
            : shared_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    frame->rax = 0u;
    if (!shared_unmap_logged) {
        shared_unmap_logged = 1;
        shared_debug("SharedMemory: mapping released\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_shared_memory(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    switch (frame->rax) {
        case SB_SYS_SHARED_MEMORY_CREATE: return shared_create(frame);
        case SB_SYS_SHARED_MEMORY_MAP: return shared_map(frame);
        case SB_SYS_SHARED_MEMORY_UNMAP: return shared_unmap(frame);
        default:
            frame->rax = shared_error(SB_SYS_ERROR_INVALID);
            return frame;
    }
}
