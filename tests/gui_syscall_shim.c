#include <stdint.h>

static uint64_t shim_display_width = 1024u;
static uint64_t shim_display_height = 768u;

uint64_t sb_syscall0(uint64_t number) {
    if (number == 3u) return (shim_display_width << 32) | (shim_display_height << 16) | (32u << 8) | 1u;
    return 0u;
}

uint64_t sb_syscall1(uint64_t number, uint64_t arg0) {
    (void)number;
    (void)arg0;
    return 0u;
}

uint64_t sb_syscall3(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    (void)number;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    return 0u;
}

uint64_t sb_syscall4(uint64_t number, uint64_t arg0, uint64_t arg1,
                     uint64_t arg2, uint64_t arg3) {
    (void)number;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return 0u;
}

uint64_t sb_get_ticks(void) { return 0u; }
uint64_t sb_process_id(void) { return 0u; }
uint64_t sb_display_info(void) { return (shim_display_width << 32) | (shim_display_height << 16); }
uint64_t sb_display_clear(uint32_t rgb) { (void)rgb; return 0u; }
uint64_t sb_display_rect(uint32_t x, uint32_t y, uint32_t width,
                         uint32_t height, uint32_t rgb) {
    (void)x; (void)y; (void)width; (void)height; (void)rgb;
    return 0u;
}
uint64_t sb_input_key(void) { return 0u; }
uint64_t sb_input_mouse(void) { return 0u; }
uint64_t sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap,
                          uint32_t rgb) {
    (void)x; (void)y; (void)bitmap; (void)rgb;
    return 0u;
}
