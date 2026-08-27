#include "syscall.h"
#include "timer.h"
#include "scheduler.h"

static uint64_t syscall_process_id(void) {
    sb_task_t *task = scheduler_current();
    return task != 0 ? task->id : 0u;
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

void syscall_init(void) {
    /* Entry mechanism is installed by the architecture-specific layer. */
}
