#include "syscall_message_queue.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "handle.h"
#include "message_queue.h"
#include "mm/heap.h"
#include <suirabox/syscall_abi.h>

_Static_assert(SB_HANDLE_TYPE_MESSAGE_QUEUE == SB_HANDLE_ABI_TYPE_MESSAGE_QUEUE,
               "kernel/public message queue handle type mismatch");
_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER >= SB_SYS_MESSAGE_QUEUE_RECEIVE,
               "public syscall max-number table is stale");
_Static_assert(SB_SYS_MESSAGE_MAX == SB_MESSAGE_MAX_BYTES,
               "kernel/public message size mismatch");

static int queue_create_logged;
static int queue_send_logged;
static int queue_receive_logged;
static int queue_short_buffer_logged;

static void queue_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void queue_debug(const char *text) {
    while (*text) queue_debug_char(*text++);
}

static uint64_t queue_error(int64_t code) { return (uint64_t)code; }

static sb_process_t *queue_current_process(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static uint64_t queue_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_STALE: return queue_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS: return queue_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_NO_SPACE: return queue_error(SB_SYS_ERROR_LIMIT);
        default: return queue_error(SB_SYS_ERROR_INVALID);
    }
}

static void queue_handle_close(void *object) {
    if (object != 0) kheap_free(object);
}

static int queue_lookup(sb_process_t *process,
                        sb_handle_t handle,
                        uint64_t rights,
                        sb_message_queue_t **queue_out) {
    if (queue_out != 0) *queue_out = 0;
    if (process == 0 || queue_out == 0) return SB_HANDLE_ERROR_INVALID;
    return sb_handle_lookup(&process->handles,
                            handle,
                            SB_HANDLE_TYPE_MESSAGE_QUEUE,
                            rights,
                            (void **)queue_out);
}

static sb_irq_frame_t *queue_create(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue =
        (sb_message_queue_t *)kheap_alloc(sizeof(*queue));
    if (queue == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    sb_message_queue_init(queue);

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int result = sb_handle_allocate(&process->handles,
                                          SB_HANDLE_TYPE_MESSAGE_QUEUE,
                                          SB_HANDLE_RIGHT_READ |
                                              SB_HANDLE_RIGHT_WRITE |
                                              SB_HANDLE_RIGHT_WAIT |
                                              SB_HANDLE_RIGHT_QUERY,
                                          queue,
                                          queue_handle_close,
                                          &handle);
    if (result != SB_HANDLE_OK) {
        queue_handle_close(queue);
        frame->rax = queue_handle_error(result);
        return frame;
    }

    frame->rax = handle;
    if (!queue_create_logged) {
        queue_create_logged = 1;
        queue_debug("MessageQueue: userspace queue handle created\r\n");
    }
    return frame;
}

static sb_irq_frame_t *queue_send(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue = 0;
    const int lookup = queue_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_WRITE,
                                    &queue);
    if (lookup != SB_HANDLE_OK || queue == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }

    const uint64_t length = frame->rdx;
    if (frame->rsi == 0u || length == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (length > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    uint8_t message[SB_SYS_MESSAGE_MAX];
    if (user_copy_from(process, message, frame->rsi, length) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    const int result = sb_message_queue_send(queue, message, (uint32_t)length);
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) {
        frame->rax = queue_error(SB_SYS_ERROR_WOULD_BLOCK);
        return frame;
    }
    if (result != SB_MESSAGE_QUEUE_OK) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    frame->rax = length;
    if (!queue_send_logged) {
        queue_send_logged = 1;
        queue_debug("MessageQueue: message enqueued from userspace\r\n");
    }
    return frame;
}

static sb_irq_frame_t *queue_receive(sb_irq_frame_t *frame) {
    sb_process_t *process = queue_current_process();
    if (process == 0) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_message_queue_t *queue = 0;
    const int lookup = queue_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_READ,
                                    &queue);
    if (lookup != SB_HANDLE_OK || queue == 0) {
        frame->rax = queue_handle_error(lookup);
        return frame;
    }

    const uint64_t capacity = frame->rdx;
    if (frame->rsi == 0u || capacity == 0u) {
        frame->rax = queue_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (capacity > SB_SYS_MESSAGE_MAX) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (user_access_validate(process,
                             frame->rsi,
                             capacity,
                             SB_USER_ACCESS_WRITE) != 0) {
        frame->rax = queue_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    uint8_t message[SB_SYS_MESSAGE_MAX];
    uint32_t received = 0u;
    uint32_t required = 0u;
    const int result = sb_message_queue_receive(queue,
                                                message,
                                                (uint32_t)capacity,
                                                &received,
                                                &required);
    if (result == SB_MESSAGE_QUEUE_WOULD_BLOCK) {
        frame->rax = queue_error(SB_SYS_ERROR_WOULD_BLOCK);
        return frame;
    }
    if (result == SB_MESSAGE_QUEUE_RANGE) {
        frame->rax = queue_error(SB_SYS_ERROR_LIMIT);
        if (!queue_short_buffer_logged) {
            queue_short_buffer_logged = 1;
            queue_debug("MessageQueue: short receive buffer preserved message\r\n");
        }
        return frame;
    }
    if (result != SB_MESSAGE_QUEUE_OK || received == 0u || received > capacity) {
        frame->rax = queue_error(SB_SYS_ERROR_IO);
        return frame;
    }
    if (user_copy_to(process, frame->rsi, message, received) != 0) {
        /* Full-range validation above makes this a consistency failure. The
         * queue item has been consumed only after the entire destination was
         * proven writable. */
        frame->rax = queue_error(SB_SYS_ERROR_IO);
        return frame;
    }

    frame->rax = received;
    if (!queue_receive_logged) {
        queue_receive_logged = 1;
        queue_debug("MessageQueue: message copied to userspace\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_message_queue(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    switch (frame->rax) {
        case SB_SYS_MESSAGE_QUEUE_CREATE: return queue_create(frame);
        case SB_SYS_MESSAGE_QUEUE_SEND: return queue_send(frame);
        case SB_SYS_MESSAGE_QUEUE_RECEIVE: return queue_receive(frame);
        default:
            frame->rax = queue_error(SB_SYS_ERROR_INVALID);
            return frame;
    }
}
