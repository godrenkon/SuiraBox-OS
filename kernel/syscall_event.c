#include "syscall_event.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "handle.h"
#include "event.h"
#include "mm/heap.h"
#include <suirabox/syscall_abi.h>

_Static_assert(SB_HANDLE_TYPE_EVENT == SB_HANDLE_ABI_TYPE_EVENT,
               "kernel/public event handle type mismatch");
_Static_assert(SB_SYS_PUBLIC_MAX_NUMBER == SB_SYS_EVENT_RESET,
               "public syscall max-number table is stale");

static int event_create_logged;
static int event_block_logged;
static int event_signal_logged;
static int event_immediate_wait_logged;
static int event_reset_logged;

static void event_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void event_debug(const char *text) {
    while (*text) event_debug_char(*text++);
}

static uint64_t event_error(int64_t code) {
    return (uint64_t)code;
}

static sb_task_t *event_current_task(void) {
    sb_task_t *task = scheduler_current();
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return task;
}

static sb_process_t *event_current_process(sb_task_t **task_out) {
    if (task_out != 0) *task_out = 0;
    sb_task_t *task = event_current_task();
    if (task == 0) return 0;
    sb_process_t *process = process_get(task->process_id);
    if (process == 0) return 0;
    if (task_out != 0) *task_out = task;
    return process;
}

static uint64_t event_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_STALE:
            return event_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS:
            return event_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_NO_SPACE:
            return event_error(SB_SYS_ERROR_LIMIT);
        case SB_HANDLE_ERROR_INVALID:
        case SB_HANDLE_ERROR_TYPE:
        default:
            return event_error(SB_SYS_ERROR_INVALID);
    }
}

static void event_handle_close(void *object) {
    sb_event_t *event = (sb_event_t *)object;
    if (event == 0) return;
    event->waiter_task_id = 0u;
    kheap_free(event);
}

static int event_lookup(sb_process_t *process,
                        sb_handle_t handle,
                        uint64_t rights,
                        sb_event_t **event_out) {
    if (event_out != 0) *event_out = 0;
    if (process == 0 || event_out == 0) return SB_HANDLE_ERROR_INVALID;
    return sb_handle_lookup(&process->handles,
                            handle,
                            SB_HANDLE_TYPE_EVENT,
                            rights,
                            (void **)event_out);
}

static void event_drop_stale_waiter(sb_event_t *event) {
    if (event == 0 || event->waiter_task_id == 0u) return;
    if (!scheduler_task_is_blocked(event->waiter_task_id)) {
        (void)sb_event_clear_waiter(event, event->waiter_task_id);
    }
}

static sb_irq_frame_t *event_create(sb_irq_frame_t *frame) {
    sb_process_t *process = event_current_process(0);
    if (process == 0 || frame->rdi > SB_EVENT_INITIAL_SIGNALED) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_event_t *event = (sb_event_t *)kheap_alloc(sizeof(*event));
    if (event == 0) {
        frame->rax = event_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    sb_event_init(event, frame->rdi == SB_EVENT_INITIAL_SIGNALED);

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int result = sb_handle_allocate(&process->handles,
                                          SB_HANDLE_TYPE_EVENT,
                                          SB_HANDLE_RIGHT_WAIT |
                                              SB_HANDLE_RIGHT_SIGNAL |
                                              SB_HANDLE_RIGHT_QUERY,
                                          event,
                                          event_handle_close,
                                          &handle);
    if (result != SB_HANDLE_OK) {
        event_handle_close(event);
        frame->rax = event_handle_error(result);
        return frame;
    }

    frame->rax = handle;
    if (!event_create_logged) {
        event_create_logged = 1;
        event_debug("Event: userspace manual-reset event created\r\n");
    }
    return frame;
}

static sb_irq_frame_t *event_wait(sb_irq_frame_t *frame) {
    sb_task_t *task = 0;
    sb_process_t *process = event_current_process(&task);
    if (process == 0 || task == 0) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_event_t *event = 0;
    const int lookup = event_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_WAIT,
                                    &event);
    if (lookup != SB_HANDLE_OK || event == 0) {
        frame->rax = event_handle_error(lookup);
        return frame;
    }

    if (sb_event_try_wait(event) == SB_EVENT_OK) {
        frame->rax = 0u;
        if (!event_immediate_wait_logged) {
            event_immediate_wait_logged = 1;
            event_debug("Event: signaled wait completed immediately\r\n");
        }
        return frame;
    }

    const uint64_t timeout_ticks = frame->rsi;
    if (timeout_ticks == 0u) {
        frame->rax = event_error(SB_SYS_ERROR_WOULD_BLOCK);
        return frame;
    }

    event_drop_stale_waiter(event);
    if (event->waiter_task_id != 0u) {
        frame->rax = event_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }

    const int register_result = sb_event_register_waiter(event, task->id);
    if (register_result == SB_EVENT_BUSY) {
        frame->rax = event_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (register_result != SB_EVENT_OK) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (event->signaled != 0u) {
        frame->rax = 0u;
        return frame;
    }

    const int block_result = scheduler_block_current_until(
        timeout_ticks,
        event_error(SB_SYS_ERROR_TIMEOUT));
    if (block_result != 0) {
        (void)sb_event_clear_waiter(event, task->id);
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    if (!event_block_logged) {
        event_block_logged = 1;
        event_debug("Event: unsignaled wait blocked with timeout\r\n");
    }
    return scheduler_reschedule(frame);
}

static sb_irq_frame_t *event_signal(sb_irq_frame_t *frame) {
    sb_process_t *process = event_current_process(0);
    if (process == 0) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_event_t *event = 0;
    const int lookup = event_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_SIGNAL,
                                    &event);
    if (lookup != SB_HANDLE_OK || event == 0) {
        frame->rax = event_handle_error(lookup);
        return frame;
    }

    event_drop_stale_waiter(event);
    uint64_t waiter_task_id = 0u;
    if (sb_event_signal(event, &waiter_task_id) != SB_EVENT_OK) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    if (waiter_task_id != 0u) {
        (void)scheduler_wake_task_with_result(waiter_task_id, 0u);
    }

    frame->rax = 0u;
    if (!event_signal_logged) {
        event_signal_logged = 1;
        event_debug("Event: manual-reset event signaled\r\n");
    }
    return frame;
}

static sb_irq_frame_t *event_reset(sb_irq_frame_t *frame) {
    sb_process_t *process = event_current_process(0);
    if (process == 0) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_event_t *event = 0;
    const int lookup = event_lookup(process,
                                    (sb_handle_t)frame->rdi,
                                    SB_HANDLE_RIGHT_SIGNAL,
                                    &event);
    if (lookup != SB_HANDLE_OK || event == 0) {
        frame->rax = event_handle_error(lookup);
        return frame;
    }

    if (sb_event_reset(event) != SB_EVENT_OK) {
        frame->rax = event_error(SB_SYS_ERROR_INVALID);
        return frame;
    }
    frame->rax = 0u;
    if (!event_reset_logged) {
        event_reset_logged = 1;
        event_debug("Event: manual-reset event reset\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_event(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    switch (frame->rax) {
        case SB_SYS_EVENT_CREATE:
            return event_create(frame);
        case SB_SYS_EVENT_WAIT:
            return event_wait(frame);
        case SB_SYS_EVENT_SIGNAL:
            return event_signal(frame);
        case SB_SYS_EVENT_RESET:
            return event_reset(frame);
        default:
            frame->rax = event_error(SB_SYS_ERROR_INVALID);
            return frame;
    }
}
