#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "process.h"
#include "process_exec.h"
#include "user_access.h"
#include <stddef.h>

#define SB_INIT_PID 1u
#define SB_VALIDATION_CHILD_PID 2u
#define SB_VALIDATION_CHILD_TID 20001u
#define SB_DEFAULT_USER_PRIORITY 128u

_Static_assert(offsetof(sb_irq_frame_t, r10) == 5u * sizeof(uint64_t),
               "syscall ABI r10 frame offset changed");
_Static_assert(offsetof(sb_irq_frame_t, r8) == 7u * sizeof(uint64_t),
               "syscall ABI r8 frame offset changed");
_Static_assert(offsetof(sb_irq_frame_t, rdi) == 8u * sizeof(uint64_t),
               "syscall ABI rdi frame offset changed");
_Static_assert(offsetof(sb_irq_frame_t, rsi) == 9u * sizeof(uint64_t),
               "syscall ABI rsi frame offset changed");
_Static_assert(offsetof(sb_irq_frame_t, rdx) == 11u * sizeof(uint64_t),
               "syscall ABI rdx frame offset changed");
_Static_assert(offsetof(sb_irq_frame_t, rax) == 14u * sizeof(uint64_t),
               "syscall ABI rax frame offset changed");
_Static_assert(SB_SYS_MAX_NUMBER == SB_SYS_HANDLE_CLOSE,
               "syscall ABI max-number table is stale");
_Static_assert(SB_HANDLE_TYPE_PROCESS == SB_HANDLE_ABI_TYPE_PROCESS,
               "kernel/public process handle type mismatch");
_Static_assert(SB_HANDLE_RIGHT_QUERY == SB_HANDLE_ABI_RIGHT_QUERY,
               "kernel/public handle rights mismatch");

static int first_user_syscall_logged;
static uint64_t first_user_task_id;
static int resumed_user_syscall_logged;
static int sleep_cycle_armed;
static uint64_t sleep_task_id;
static int woke_user_syscall_logged;
static int wait_cycle_armed;
static uint64_t wait_task_id;
static int wait_resume_logged;
static int child_pid_syscall_logged;
static int child_spawn_logged;
static int abi_version_logged;
static int invalid_pointer_logged;
static int writable_pointer_logged;
static int readonly_pointer_logged;
static int handle_open_logged;
static int handle_query_logged;
static int handle_close_logged;
static int stale_handle_logged;

static void syscall_debug_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void syscall_debug(const char *s) {
    while (*s) syscall_debug_char(*s++);
}

static uint64_t syscall_error(int64_t code) {
    return (uint64_t)code;
}

static uint64_t syscall_handle_error(int result) {
    switch (result) {
        case SB_HANDLE_ERROR_NO_SPACE:
            return syscall_error(SB_SYS_ERROR_LIMIT);
        case SB_HANDLE_ERROR_STALE:
            return syscall_error(SB_SYS_ERROR_STALE);
        case SB_HANDLE_ERROR_RIGHTS:
            return syscall_error(SB_SYS_ERROR_RIGHTS);
        case SB_HANDLE_ERROR_INVALID:
        case SB_HANDLE_ERROR_TYPE:
        default:
            return syscall_error(SB_SYS_ERROR_INVALID);
    }
}

static uint64_t syscall_process_id(void) {
    sb_task_t *task = scheduler_current();
    return task != 0 ? task->process_id : 0u;
}

uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;

    switch (number) {
        case SB_SYS_GET_TICKS:
            return timer_ticks();
        case SB_SYS_PROCESS_ID:
            return syscall_process_id();
        case SB_SYS_ABI_VERSION:
            return SB_SYSCALL_ABI_VERSION;
        case SB_SYS_EXIT:
        case SB_SYS_SPAWN:
        case SB_SYS_WAIT_PROCESS:
        case SB_SYS_LOG_WRITE:
        case SB_SYS_ABI_INFO:
        case SB_SYS_PROCESS_OPEN_SELF:
        case SB_SYS_HANDLE_INFO:
        case SB_SYS_HANDLE_CLOSE:
            return syscall_error(SB_SYS_ERROR_INVALID);
        case SB_SYS_SLEEP:
            return scheduler_sleep_current(arg0) == 0
                ? 0u : syscall_error(SB_SYS_ERROR_INVALID);
        default:
            return syscall_error(SB_SYS_ERROR_INVALID);
    }
}

static sb_process_t *syscall_current_process(const sb_task_t *task) {
    if (task == 0 || task->user_task == 0u || task->process_id == 0u) return 0;
    return process_get(task->process_id);
}

static sb_irq_frame_t *syscall_log_write(sb_irq_frame_t *frame, sb_task_t *task) {
    sb_process_t *process = syscall_current_process(task);
    if (process == 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const uint64_t length = frame->rsi;
    if (length > SB_SYS_LOG_MAX) {
        frame->rax = syscall_error(SB_SYS_ERROR_LIMIT);
        return frame;
    }
    if (length == 0u) {
        frame->rax = 0u;
        return frame;
    }

    uint8_t buffer[SB_SYS_LOG_MAX];
    if (user_copy_from(process, buffer, frame->rdi, length) != 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_FAULT);
        if (!invalid_pointer_logged) {
            invalid_pointer_logged = 1;
            syscall_debug("Syscall: invalid user pointer rejected\r\n");
        }
        return frame;
    }

    for (uint64_t i = 0u; i < length; ++i) syscall_debug_char((char)buffer[i]);
    frame->rax = length;
    return frame;
}

static sb_irq_frame_t *syscall_abi_info(sb_irq_frame_t *frame, sb_task_t *task) {
    sb_process_t *process = syscall_current_process(task);
    if (process == 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const sb_syscall_abi_info_t info = {
        .abi_version = SB_SYSCALL_ABI_VERSION,
        .max_syscall_number = SB_SYS_MAX_NUMBER,
    };

    if (user_copy_to(process, frame->rdi, &info, sizeof(info)) != 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_FAULT);
        if (!readonly_pointer_logged) {
            readonly_pointer_logged = 1;
            syscall_debug("Syscall: read-only user pointer write rejected\r\n");
        }
        return frame;
    }

    frame->rax = 0u;
    if (!writable_pointer_logged) {
        writable_pointer_logged = 1;
        syscall_debug("Syscall: writable user pointer copy OK\r\n");
    }
    return frame;
}

static sb_irq_frame_t *syscall_process_open_self(sb_irq_frame_t *frame,
                                                  sb_task_t *task) {
    sb_process_t *process = syscall_current_process(task);
    if (process == 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_handle_t handle = SB_HANDLE_INVALID;
    const int result = sb_handle_allocate(&process->handles,
                                          SB_HANDLE_TYPE_PROCESS,
                                          SB_HANDLE_RIGHT_QUERY,
                                          process,
                                          0,
                                          &handle);
    if (result != SB_HANDLE_OK) {
        frame->rax = syscall_handle_error(result);
        return frame;
    }

    frame->rax = handle;
    if (!handle_open_logged) {
        handle_open_logged = 1;
        syscall_debug("Handle: userspace process handle opened\r\n");
    }
    return frame;
}

static sb_irq_frame_t *syscall_handle_info(sb_irq_frame_t *frame,
                                            sb_task_t *task) {
    sb_process_t *process = syscall_current_process(task);
    if (process == 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    sb_handle_info_t info;
    const int result = sb_handle_query(&process->handles,
                                       (sb_handle_t)frame->rdi,
                                       SB_HANDLE_RIGHT_QUERY,
                                       &info);
    if (result != SB_HANDLE_OK) {
        frame->rax = syscall_handle_error(result);
        if (result == SB_HANDLE_ERROR_STALE && !stale_handle_logged) {
            stale_handle_logged = 1;
            syscall_debug("Handle: stale generation rejected\r\n");
        }
        return frame;
    }

    if (user_copy_to(process, frame->rsi, &info, sizeof(info)) != 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_FAULT);
        return frame;
    }

    frame->rax = 0u;
    if (!handle_query_logged) {
        handle_query_logged = 1;
        syscall_debug("Handle: type and rights query OK\r\n");
    }
    return frame;
}

static sb_irq_frame_t *syscall_handle_close(sb_irq_frame_t *frame,
                                             sb_task_t *task) {
    sb_process_t *process = syscall_current_process(task);
    if (process == 0) {
        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        return frame;
    }

    const int result = sb_handle_close(&process->handles,
                                       (sb_handle_t)frame->rdi);
    frame->rax = result == SB_HANDLE_OK ? 0u : syscall_handle_error(result);
    if (result == SB_HANDLE_OK && !handle_close_logged) {
        handle_close_logged = 1;
        syscall_debug("Handle: close invalidated generation\r\n");
    } else if (result == SB_HANDLE_ERROR_STALE && !stale_handle_logged) {
        stale_handle_logged = 1;
        syscall_debug("Handle: stale generation rejected\r\n");
    }
    return frame;
}

sb_irq_frame_t *sb_syscall_dispatch_frame(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;

    sb_task_t *task = scheduler_current();
    const uint64_t number = frame->rax;

    if (task != 0 && task->user_task != 0u) {
        if (!first_user_syscall_logged) {
            first_user_syscall_logged = 1;
            first_user_task_id = task->id;
            syscall_debug("Userspace: first syscall reached kernel\r\n");
        }
        if (!resumed_user_syscall_logged && task->id == first_user_task_id &&
            task->dispatch_count >= 2u) {
            resumed_user_syscall_logged = 1;
            syscall_debug("Userspace: resumed user thread reached syscall\r\n");
        }
    }

    if (wait_cycle_armed && !wait_resume_logged && number != SB_SYS_WAIT_PROCESS &&
        task != 0 && task->id == wait_task_id && task->user_task != 0u) {
        wait_resume_logged = 1;
        wait_cycle_armed = 0;
        wait_task_id = 0u;
        syscall_debug("Userspace: parent wait resumed after child exit\r\n");
    }

    if (number == SB_SYS_LOG_WRITE) {
        return syscall_log_write(frame, task);
    }

    if (number == SB_SYS_ABI_INFO) {
        return syscall_abi_info(frame, task);
    }

    if (number == SB_SYS_PROCESS_OPEN_SELF) {
        return syscall_process_open_self(frame, task);
    }

    if (number == SB_SYS_HANDLE_INFO) {
        return syscall_handle_info(frame, task);
    }

    if (number == SB_SYS_HANDLE_CLOSE) {
        return syscall_handle_close(frame, task);
    }

    if (number == SB_SYS_SLEEP) {
        const int result = scheduler_sleep_current(frame->rdi);
        frame->rax = result == 0 ? 0u : syscall_error(SB_SYS_ERROR_INVALID);
        if (result == 0 && task != 0) {
            sleep_cycle_armed = 1;
            sleep_task_id = task->id;
            syscall_debug("Userspace: sleep syscall requested\r\n");
            return scheduler_reschedule(frame);
        }
        return frame;
    }

    if (number == SB_SYS_SPAWN) {
        if (task == 0 || task->user_task == 0u || task->process_id != SB_INIT_PID ||
            frame->rdi != SB_SPAWN_IMAGE_CHILD || process_get(SB_VALIDATION_CHILD_PID) != 0) {
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }

        sb_process_image_t child_image;
        sb_process_t *child = process_spawn_registered_boot_module("user-child",
                                                                   SB_VALIDATION_CHILD_PID,
                                                                   task->process_id,
                                                                   SB_VALIDATION_CHILD_TID,
                                                                   SB_DEFAULT_USER_PRIORITY,
                                                                   &child_image);
        if (child == 0) {
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }

        frame->rax = child->pid;
        if (!child_spawn_logged) {
            child_spawn_logged = 1;
            syscall_debug("Userspace: spawn syscall created child process\r\n");
        }
        return frame;
    }

    if (number == SB_SYS_WAIT_PROCESS) {
        if (task == 0 || task->user_task == 0u || task->process_id == 0u) {
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }

        int64_t exit_code = 0;
        const uint64_t child_pid = frame->rdi;
        const int wait_result = process_wait_child(task->process_id,
                                                   child_pid,
                                                   task->id,
                                                   &exit_code);
        if (wait_result == 0) {
            frame->rax = (uint64_t)exit_code;
            return frame;
        }
        if (wait_result < 0 || scheduler_block_current() != 0) {
            if (wait_result > 0) (void)process_cancel_wait(child_pid, task->id);
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }

        frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
        wait_cycle_armed = 1;
        wait_task_id = task->id;
        syscall_debug("Userspace: wait syscall blocked for child\r\n");
        return scheduler_reschedule(frame);
    }

    if (number == SB_SYS_EXIT) {
        if (task == 0 || task->user_task == 0u || task->process_id == 0u ||
            process_get(task->process_id) == 0) {
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }

        const uint64_t pid = task->process_id;
        const int64_t exit_code = (int64_t)frame->rdi;
        if (scheduler_exit_current_process(exit_code) != 0) {
            frame->rax = syscall_error(SB_SYS_ERROR_INVALID);
            return frame;
        }
        if (process_mark_exited(pid, exit_code) != 0) {
            syscall_debug("Process: exit state consistency failure\r\n");
        }

        frame->rax = 0u;
        if (pid == SB_VALIDATION_CHILD_PID) {
            syscall_debug("Userspace: child requested process exit\r\n");
        }
        return scheduler_reschedule(frame);
    }

    frame->rax = syscall_dispatch(number,
                                  frame->rdi,
                                  frame->rsi,
                                  frame->rdx,
                                  frame->r10,
                                  frame->r8);

    if (number == SB_SYS_ABI_VERSION && task != 0 && task->process_id == SB_INIT_PID &&
        frame->rax == SB_SYSCALL_ABI_VERSION && !abi_version_logged) {
        abi_version_logged = 1;
        syscall_debug("Syscall: ABI v1 userspace probe OK\r\n");
    }

    if (number == SB_SYS_PROCESS_ID && task != 0 &&
        task->process_id == SB_VALIDATION_CHILD_PID &&
        frame->rax == SB_VALIDATION_CHILD_PID && !child_pid_syscall_logged) {
        child_pid_syscall_logged = 1;
        syscall_debug("Userspace: child process PID syscall OK\r\n");
    }

    if (sleep_cycle_armed && !woke_user_syscall_logged &&
        number != SB_SYS_SLEEP && task != 0 && task->id == sleep_task_id &&
        task->user_task != 0u) {
        woke_user_syscall_logged = 1;
        sleep_cycle_armed = 0;
        sleep_task_id = 0u;
        syscall_debug("Userspace: woke sleeping user thread reached syscall\r\n");
    }

    return frame;
}

void syscall_init(void) {
    first_user_syscall_logged = 0;
    first_user_task_id = 0u;
    resumed_user_syscall_logged = 0;
    sleep_cycle_armed = 0;
    sleep_task_id = 0u;
    woke_user_syscall_logged = 0;
    wait_cycle_armed = 0;
    wait_task_id = 0u;
    wait_resume_logged = 0;
    child_pid_syscall_logged = 0;
    child_spawn_logged = 0;
    abi_version_logged = 0;
    invalid_pointer_logged = 0;
    writable_pointer_logged = 0;
    readonly_pointer_logged = 0;
    handle_open_logged = 0;
    handle_query_logged = 0;
    handle_close_logged = 0;
    stale_handle_logged = 0;
}
