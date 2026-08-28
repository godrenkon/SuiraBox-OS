#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "framebuffer.h"

static uint64_t syscall_process_id(void) {
    sb_task_t *task = scheduler_current();
    return task != 0 ? task->id : 0u;
}

static uint64_t syscall_display_info(void) {
    const sb_framebuffer_info_t *fb;
    if (!sb_framebuffer_available()) return 0u;
    fb = sb_framebuffer_info();
    if (fb == 0 || fb->width > UINT32_MAX || fb->height > UINT16_MAX || fb->bits_per_pixel > UINT8_MAX)
        return 0u;
    return (fb->width << 32) | (fb->height << 16) | ((uint64_t)fb->bits_per_pixel << 8) | 1u;
}

uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    switch (number) {
        case SB_SYS_GET_TICKS:
            return timer_ticks();
        case SB_SYS_PROCESS_ID:
            return syscall_process_id();
        case SB_SYS_EXIT:
            return 0u;
        case SB_SYS_DISPLAY_INFO:
            return syscall_display_info();
        case SB_SYS_DISPLAY_CLEAR:
            return sb_framebuffer_clear((uint8_t)((arg0 >> 16) & 0xFFu),
                                        (uint8_t)((arg0 >> 8) & 0xFFu),
                                        (uint8_t)(arg0 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_DISPLAY_RECT:
            if (arg0 > UINT32_MAX || arg1 > UINT32_MAX || arg2 > UINT32_MAX || arg3 > UINT32_MAX || arg4 > UINT32_MAX)
                return UINT64_MAX;
            return sb_framebuffer_fill_rect((uint32_t)arg0, (uint32_t)arg1,
                                            (uint32_t)arg2, (uint32_t)arg3,
                                            (uint8_t)((arg4 >> 16) & 0xFFu),
                                            (uint8_t)((arg4 >> 8) & 0xFFu),
                                            (uint8_t)(arg4 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        default:
            return UINT64_MAX;
    }
}

uint64_t sb_syscall_dispatch_entry(uint64_t number, uint64_t arg0, uint64_t arg1,
                                   uint64_t arg2, uint64_t arg3) {
    return syscall_dispatch(number, arg0, arg1, arg2, arg3, 0u);
}

void syscall_init(void) {
    /* Architecture-specific entry is installed during kernel initialization. */
}
