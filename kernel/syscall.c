#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "process.h"
#include "process_exec.h"

#define SB_INIT_PID 1u
#define SB_VALIDATION_CHILD_PID 2u
#define SB_VALIDATION_CHILD_TID 20001u
#define SB_DEFAULT_USER_PRIORITY 128u

static int first_user_syscall_logged;
static uint64_t first_user_task_id;
static int resumed_user_syscall_logged;
static int sleep_cycle_armed;
static uint64_t sleep_task_id;
static int woke_user_syscall_logged;
static int child_pid_syscall_logged;
static int child_spawn_logged;

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
        case SB_SYS_EXIT:
        case SB_SYS_SPAWN:
            /* These require scheduler/process state and are handled by the
             * complete saved-frame path below. */
            return UINT64_MAX;
        case SB_SYS_SLEEP:
            return scheduler_sleep_current(arg0) == 0 ? 0u : UINT64_MAX;
        default:
            return UINT64_MAX;
    }
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

    if (number == SB_SYS_SLEEP) {
        const int result = scheduler_sleep_current(frame->rdi);
        frame->rax = result == 0 ? 0u : UINT64_MAX;
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
            frame->rax = UINT64_MAX;
            return frame;
        }

        sb_process_image_t child_image;
        sb_process_t *child = process_spawn_registered_boot_module("user-child",
                                                                   SB_VALIDATION_CHILD_PID,
                                                                   SB_VALIDATION_CHILD_TID,
                                                                   SB_DEFAULT_USER_PRIORITY,
                                                                   &child_image);
        if (child == 0) {
            frame->rax = UINT64_MAX;
            return frame;
        }

        frame->rax = child->pid;
        if (!child_spawn_logged) {
            child_spawn_logged = 1;
            syscall_debug("Userspace: spawn syscall created child process\r\n");
        }
        return frame;
    }

    if (number == SB_SYS_EXIT) {
        if (task == 0 || task->user_task == 0u || task->process_id == 0u ||
            process_get(task->process_id) == 0) {
            frame->rax = UINT64_MAX;
            return frame;
        }

        const uint64_t pid = task->process_id;
        const int64_t exit_code = (int64_t)frame->rdi;
        if (scheduler_exit_current_process(exit_code) != 0) {
            frame->rax = UINT64_MAX;
            return frame;
        }
        if (process_mark_exited(pid, exit_code) != 0) {
            /* The process object was validated above, so reaching this path
             * indicates an internal consistency failure. Never return to a
             * task whose scheduler state is already EXITED. */
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
    child_pid_syscall_logged = 0;
    child_spawn_logged = 0;
}
