#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "framebuffer.h"

static uint8_t io_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint64_t syscall_process_id(void) {
    sb_task_t *task = scheduler_current();
    return task != 0 ? task->id : 0u;
}

static uint64_t syscall_display_info(void) {
    const sb_framebuffer_info_t *fb;
    if (!sb_framebuffer_available()) return 0u;
    fb = sb_framebuffer_info();
    if (fb == 0 || fb->height > UINT16_MAX) return 0u;
    return ((uint64_t)fb->width << 32) | ((uint64_t)fb->height << 16) |
           ((uint64_t)fb->bits_per_pixel << 8) | 1u;
}

static uint64_t syscall_input_key(void) {
    if ((io_in8(0x64u) & 0x01u) == 0u) return 0u;
    return (uint64_t)io_in8(0x60u);
}

static uint64_t syscall_display_glyph(uint64_t x, uint64_t y, uint64_t bitmap, uint64_t color) {
    uint32_t px;
    uint32_t py;
    uint8_t red = (uint8_t)((color >> 16) & 0xFFu);
    uint8_t green = (uint8_t)((color >> 8) & 0xFFu);
    uint8_t blue = (uint8_t)(color & 0xFFu);

    if (x > UINT32_MAX - 7u || y > UINT32_MAX - 7u) return UINT64_MAX;
    for (py = 0u; py < 8u; ++py) {
        const uint8_t row = (uint8_t)(bitmap >> (py * 8u));
        for (px = 0u; px < 8u; ++px) {
            if ((row & (uint8_t)(1u << (7u - px))) != 0u) {
                if (sb_framebuffer_draw_pixel((uint32_t)x + px, (uint32_t)y + py,
                                              red, green, blue) != 0)
                    return UINT64_MAX;
            }
        }
    }
    return 0u;
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
        case SB_SYS_INPUT_KEY:
            return syscall_input_key();
        case SB_SYS_DISPLAY_GLYPH:
            return syscall_display_glyph(arg0, arg1, arg2, arg3);
        default:
            return UINT64_MAX;
    }
}

uint64_t sb_syscall_dispatch_entry(uint64_t number, uint64_t arg0, uint64_t arg1,
                                   uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    return syscall_dispatch(number, arg0, arg1, arg2, arg3, arg4);
}

void syscall_init(void) {
    /* Architecture-specific entry is installed during kernel initialization. */
}
