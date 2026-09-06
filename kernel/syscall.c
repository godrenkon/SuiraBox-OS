#include "syscall.h"
#include "timer.h"
#include "scheduler.h"

static int first_user_syscall_logged;
static int resumed_user_syscall_logged;

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
    (void)arg0;
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
            return 0u;
        default:
            return UINT64_MAX;
    }
}

uint64_t sb_syscall_dispatch_entry(uint64_t number, uint64_t arg0, uint64_t arg1,
                                   uint64_t arg2, uint64_t arg3) {
    sb_task_t *task = scheduler_current();
    if (task != 0 && task->user_task != 0u) {
        if (!first_user_syscall_logged) {
            first_user_syscall_logged = 1;
            syscall_debug("Userspace: first syscall reached kernel\r\n");
        }
        if (!resumed_user_syscall_logged && task->dispatch_count >= 2u) {
            resumed_user_syscall_logged = 1;
            syscall_debug("Userspace: resumed user thread reached syscall\r\n");
        }
    }
    return syscall_dispatch(number, arg0, arg1, arg2, arg3, 0u);
}

void syscall_init(void) {
    first_user_syscall_logged = 0;
    resumed_user_syscall_logged = 0;
}
