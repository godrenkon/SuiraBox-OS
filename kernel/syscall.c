#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "framebuffer.h"
#include "storage.h"
#include "config_store.h"

static uint8_t io_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void io_out8(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int ps2_wait_input_clear(void) {
    uint32_t timeout = 100000u;
    while ((io_in8(0x64u) & 0x02u) != 0u && timeout-- != 0u) { }
    return timeout != 0u;
}

static int ps2_wait_output_ready(void) {
    uint32_t timeout = 100000u;
    while ((io_in8(0x64u) & 0x01u) == 0u && timeout-- != 0u) { }
    return timeout != 0u;
}

static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index;
static uint8_t mouse_initialized;
static uint8_t mouse_init_attempted;

static void syscall_idle(void) {
    __asm__ volatile ("sti\n\thlt\n\tcli" ::: "memory");
}

static void mouse_init(void) {
    uint8_t response;
    if (mouse_initialized || mouse_init_attempted) return;
    mouse_init_attempted = 1u;
    if (!ps2_wait_input_clear()) return;
    io_out8(0x64u, 0xA8u);
    if (!ps2_wait_input_clear()) return;
    io_out8(0x64u, 0xD4u);
    if (!ps2_wait_input_clear()) return;
    io_out8(0x60u, 0xF4u);
    if (ps2_wait_output_ready()) {
        response = io_in8(0x60u);
        if (response == 0xFAu) mouse_initialized = 1u;
    }
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
    const uint8_t status = io_in8(0x64u);
    if ((status & 0x01u) == 0u || (status & 0x20u) != 0u) return 0u;
    return (uint64_t)io_in8(0x60u);
}

static uint64_t syscall_input_mouse(void) {
    uint8_t status;
    uint8_t byte;
    uint32_t packet;

    if (!mouse_initialized) mouse_init();
    if (!mouse_initialized) {
        syscall_idle();
        return 0u;
    }
    status = io_in8(0x64u);
    if ((status & 0x01u) == 0u || (status & 0x20u) == 0u) {
        if ((status & 0x01u) != 0u && (status & 0x20u) == 0u) return 0u;
        syscall_idle();
        return 0u;
    }
    byte = io_in8(0x60u);

    if (mouse_packet_index == 0u && (byte & 0x08u) == 0u) return 0u;
    mouse_packet[mouse_packet_index++] = byte;
    if (mouse_packet_index < 3u) return 0u;

    mouse_packet_index = 0u;
    packet = 1u | ((uint32_t)(mouse_packet[0] & 0x07u) << 8) |
             ((uint32_t)mouse_packet[1] << 16) |
             ((uint32_t)mouse_packet[2] << 24);
    return packet;
}

static uint64_t syscall_config_get(void) {
    sb_config_store_record_t record;
    if (!sb_config_store_get(&record)) return 0u;
    return 1u | ((uint64_t)record.language << 8) |
           ((uint64_t)record.optional_enabled_mask << 16);
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
            if (arg0 > UINT32_MAX - 7u || arg1 > UINT32_MAX - 7u || arg3 > UINT32_MAX)
                return UINT64_MAX;
            return sb_framebuffer_draw_glyph8((uint32_t)arg0, (uint32_t)arg1, arg2,
                                              (uint8_t)((arg3 >> 16) & 0xFFu),
                                              (uint8_t)((arg3 >> 8) & 0xFFu),
                                              (uint8_t)(arg3 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_DISPLAY_GLYPH_PAIR:
            if (arg0 > UINT32_MAX - 17u || arg1 > UINT32_MAX - 7u || arg4 > UINT32_MAX)
                return UINT64_MAX;
            if (sb_framebuffer_draw_glyph8((uint32_t)arg0, (uint32_t)arg1, arg2,
                                           (uint8_t)((arg4 >> 16) & 0xFFu),
                                           (uint8_t)((arg4 >> 8) & 0xFFu),
                                           (uint8_t)(arg4 & 0xFFu)) != 0)
                return UINT64_MAX;
            return sb_framebuffer_draw_glyph8((uint32_t)arg0 + 10u, (uint32_t)arg1, arg3,
                                              (uint8_t)((arg4 >> 16) & 0xFFu),
                                              (uint8_t)((arg4 >> 8) & 0xFFu),
                                              (uint8_t)(arg4 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_INPUT_MOUSE:
            return syscall_input_mouse();
        case SB_SYS_CONFIG_GET:
            return syscall_config_get();
        case SB_SYS_CONFIG_SET: {
            const uint32_t language = (uint32_t)arg0;
            const uint32_t optional_enabled_mask = (uint32_t)arg1;
            if (language > 3u ||
                (optional_enabled_mask != SB_CONFIG_SET_KEEP_OPTIONS &&
                 optional_enabled_mask > SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED))
                return UINT64_MAX;
            if (!sb_storage_ready()) return SB_CONFIG_SET_VOLATILE;
            if (sb_config_store_set((uint8_t)language, optional_enabled_mask) != 0)
                return UINT64_MAX;
            return 0u;
        }
        case SB_SYS_YIELD:
            syscall_idle();
            return 0u;
        default:
            return UINT64_MAX;
    }
}

uint64_t sb_syscall_dispatch_entry(uint64_t number, uint64_t arg0, uint64_t arg1,
                                   uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    return syscall_dispatch(number, arg0, arg1, arg2, arg3, arg4);
}

void syscall_init(void) {
    (void)sb_storage_init();
    mouse_init();
}
