#include "syscall_pipe.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "handle.h"
#include "pipe.h"
#include "mm/heap.h"
#include <suirabox/syscall_abi.h>

_Static_assert(SB_HANDLE_TYPE_PIPE == SB_HANDLE_ABI_TYPE_PIPE,
               "kernel/public pipe handle type mismatch");
_Static_assert(sizeof(sb_pipe_handles_t) == SB_PIPE_HANDLES_SIZE,
               "pipe handle pair ABI size mismatch");

typedef enum {
    SB_PIPE_ENDPOINT_READER = 1,
    SB_PIPE_ENDPOINT_WRITER = 2,
} sb_pipe_endpoint_role_t;

typedef struct {
    sb_pipe_t pipe;
    uint32_t endpoint_refs;
} sb_pipe_shared_t;

typedef struct {
    sb_pipe_shared_t *shared;
    sb_pipe_endpoint_role_t role;
} sb_pipe_endpoint_t;

static int pipe_create_logged;
static int pipe_write_logged;
static int pipe_read_logged;
static int pipe_eof_logged;

static void pipe_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void pipe_debug(const char *text) {
    while (*text) pipe_debug_char(*text++);
}

static uint64_t pipe_error(int64_t code) {
    return (uint64_t)code;
}

static sb_process_t *pipe_current_process(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static uint64_t pipe_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_STALE:
            return pipe_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS:
            return pipe_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_NO_SPACE:
            return pipe_error(SB_SYS_ERROR_LIMIT);
        case SB_HANDLE_ERROR_INVALID:
        case SB_HANDLE_ERROR_TYPE:
        default:
            return pipe_error(SB_SYS_ERROR_INVALID);
    }
}

static uint64_t pipe_core_error(int result) {
    switch (result) {
        case SB_PIPE_WOULD_BLOCK:
            return pipe_error(SB_SYS_ERROR_WOULD_BLOCK);
        case SB_PIPE_CLOSED:
            return pipe_error(SB_SYS_ERROR_CLOSED);
        case SB_PIPE_INVALID:
        default:
            return pipe_error(SB_SYS_ERROR_INVALID);
    }
}

static void pipe_endpoint_close(void *object) {
    sb_pipe_endpoint_t *endpoint = (sb_pipe_endpoint_t *)object;
    if (endpoint == 0) return;

    sb_pipe_shared_t *shared = endpoint->shared;
    if (shared != 0) {
        if (endpoint->role == SB_PIPE_ENDPOINT_READER) {
            (void)sb_pipe_close_reader(&shared->pipe);
        } else if (endpoint->role == SB_PIPE_ENDPOINT_WRITER) {
            (void)sb_pipe_close_writer(&shared->pipe);
        }
        if (shared->endpoint_refs != 0u) {
            --shared->endpoint_refs;
            if (shared->endpoint_refs == 0u) kheap_free(shared);
        }
    }
    endpoint->shared = 0;
    kheap_free(endpoint);
}

static sb_pipe_endpoint_t *pipe_endpoint_alloc(sb_pipe_shared_t *shared,
                                                sb_pipe_endpoint_role_t role) {
    sb_pipe_endpoint_t *endpoint =
        (sb_pipe_endpoint_t *)kheap_alloc(sizeof(*endpoint));
    if (endpoint == 0) return 0;
    endpoint->shared = shared;
    endpoint->role = role;
    return endpoint;
}

static sb_irq_frame_t *pipe_create(sb_irq_frame_t *frame) {
    sb_process_t *process = pipe_current_process();
    if (process == 0 || frame->rdi == 0u) {
        frame->rax = pipe_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (user_access_validate(process,
                             frame->rdi,
                             sizeof(sb_pipe_handles_t),
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    sb_pipe_shared_t *shared =
        (sb_pipe_shared_t *)kheap_alloc(sizeof(*shared));
    if (shared == 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    sb_pipe_init(&shared->pipe);
    shared->endpoint_refs = 2u;
    if (sb_pipe_open_reader(&shared->pipe) != SB_PIPE_OK ||
        sb_pipe_open_writer(&shared->pipe) != SB_PIPE_OK) {
        kheap_free(shared);
        frame->rax = pipe_error(SB_SYS_ERROR_IO);
        return frame;
    }

    sb_pipe_endpoint_t *reader =
        pipe_endpoint_alloc(shared, SB_PIPE_ENDPOINT_READER);
    sb_pipe_endpoint_t *writer =
        pipe_endpoint_alloc(shared, SB_PIPE_ENDPOINT_WRITER);
    if (reader == 0 || writer == 0) {
        if (reader != 0) pipe_endpoint_close(reader);
        if (writer != 0) pipe_endpoint_close(writer);
        if (reader == 0 && writer == 0) kheap_free(shared);
        else if (reader == 0 || writer == 0) {
            /* One endpoint never existed, so release its reserved reference. */
            if (shared->endpoint_refs != 0u) {
                --shared->endpoint_refs;
                if (shared->endpoint_refs == 0u) kheap_free(shared);
            }
        }
        frame->rax = pipe_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    sb_handle_t read_handle = SB_HANDLE_INVALID;
    sb_handle_t write_handle = SB_HANDLE_INVALID;
    int result = sb_handle_allocate(&process->handles,
                                    SB_HANDLE_TYPE_PIPE,
                                    SB_HANDLE_RIGHT_READ |
                                        SB_HANDLE_RIGHT_WAIT |
                                        SB_HANDLE_RIGHT_QUERY,
                                    reader,
                                    pipe_endpoint_close,
                                    &read_handle);
    if (result != SB_HANDLE_OK) {
        pipe_endpoint_close(reader);
        pipe_endpoint_close(writer);
        frame->rax = pipe_handle_error(result);
        return frame;
    }

    result = sb_handle_allocate(&process->handles,
                                SB_HANDLE_TYPE_PIPE,
                                SB_HANDLE_RIGHT_WRITE |
                                    SB_HANDLE_RIGHT_WAIT |
                                    SB_HANDLE_RIGHT_QUERY,
                                writer,
                                pipe_endpoint_close,
                                &write_handle);
    if (result != SB_HANDLE_OK) {
        (void)sb_handle_close(&process->handles, read_handle);
        pipe_endpoint_close(writer);
        frame->rax = pipe_handle_error(result);
        return frame;
    }

    const sb_pipe_handles_t handles = {
        .read_handle = read_handle,
        .write_handle = write_handle,
    };
    if (user_copy_to(process, frame->rdi, &handles, sizeof(handles)) != 0) {
        (void)sb_handle_close(&process->handles, write_handle);
        (void)sb_handle_close(&process->handles, read_handle);
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    frame->rax = 0u;
    if (!pipe_create_logged) {
        pipe_create_logged = 1;
        pipe_debug("Pipe: userspace pipe handles created\r\n");
    }
    return frame;
}

static sb_irq_frame_t *pipe_read(sb_irq_frame_t *frame) {
    sb_process_t *process = pipe_current_process();
    if (process == 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_pipe_endpoint_t *endpoint = 0;
    const int lookup = sb_handle_lookup(&process->handles,
                                        (sb_handle_t)frame->rdi,
                                        SB_HANDLE_TYPE_PIPE,
                                        SB_HANDLE_RIGHT_READ,
                                        (void **)&endpoint);
    if (lookup != SB_HANDLE_OK || endpoint == 0 || endpoint->shared == 0 ||
        endpoint->role != SB_PIPE_ENDPOINT_READER) {
        frame->rax = lookup != SB_HANDLE_OK
            ? pipe_handle_error(lookup) : pipe_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t length = frame->rdx;
    if (length > SB_SYS_PIPE_IO_MAX) {
        frame->rax = pipe_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (length == 0u) {
        frame->rax = 0u;
        return frame;
    }
    if (frame->rsi == 0u ||
        user_access_validate(process,
                             frame->rsi,
                             length,
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint8_t buffer[SB_SYS_PIPE_IO_MAX];
    uint32_t bytes_read = 0u;
    const int result = sb_pipe_read(&endpoint->shared->pipe,
                                    buffer,
                                    (uint32_t)length,
                                    &bytes_read);
    if (result != SB_PIPE_OK) {
        frame->rax = pipe_core_error(result);
        return frame;
    }
    /* The full user range was validated before consuming pipe data. On the
     * current single-CPU kernel no mapping can race this second checked copy. */
    if (bytes_read != 0u &&
        user_copy_to(process, frame->rsi, buffer, bytes_read) != 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    frame->rax = bytes_read;
    if (bytes_read != 0u && !pipe_read_logged) {
        pipe_read_logged = 1;
        pipe_debug("Pipe: data read to userspace\r\n");
    } else if (bytes_read == 0u && !pipe_eof_logged) {
        pipe_eof_logged = 1;
        pipe_debug("Pipe: EOF observed after writer close\r\n");
    }
    return frame;
}

static sb_irq_frame_t *pipe_write(sb_irq_frame_t *frame) {
    sb_process_t *process = pipe_current_process();
    if (process == 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_pipe_endpoint_t *endpoint = 0;
    const int lookup = sb_handle_lookup(&process->handles,
                                        (sb_handle_t)frame->rdi,
                                        SB_HANDLE_TYPE_PIPE,
                                        SB_HANDLE_RIGHT_WRITE,
                                        (void **)&endpoint);
    if (lookup != SB_HANDLE_OK || endpoint == 0 || endpoint->shared == 0 ||
        endpoint->role != SB_PIPE_ENDPOINT_WRITER) {
        frame->rax = lookup != SB_HANDLE_OK
            ? pipe_handle_error(lookup) : pipe_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t length = frame->rdx;
    if (length > SB_SYS_PIPE_IO_MAX) {
        frame->rax = pipe_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (length == 0u) {
        frame->rax = 0u;
        return frame;
    }
    if (frame->rsi == 0u) {
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint8_t buffer[SB_SYS_PIPE_IO_MAX];
    if (user_copy_from(process, buffer, frame->rsi, length) != 0) {
        frame->rax = pipe_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint32_t bytes_written = 0u;
    const int result = sb_pipe_write(&endpoint->shared->pipe,
                                     buffer,
                                     (uint32_t)length,
                                     &bytes_written);
    if (result != SB_PIPE_OK) {
        frame->rax = pipe_core_error(result);
        return frame;
    }

    frame->rax = bytes_written;
    if (bytes_written != 0u && !pipe_write_logged) {
        pipe_write_logged = 1;
        pipe_debug("Pipe: data written from userspace\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_pipe(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    switch (frame->rax) {
        case SB_SYS_PIPE_CREATE:
            return pipe_create(frame);
        case SB_SYS_PIPE_READ:
            return pipe_read(frame);
        case SB_SYS_PIPE_WRITE:
            return pipe_write(frame);
        default:
            frame->rax = pipe_error(SB_SYS_ERROR_INVALID);
            return frame;
    }
}
