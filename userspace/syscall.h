#ifndef SB_USER_SYSCALL_H
#define SB_USER_SYSCALL_H

#include <stdint.h>

#define SB_SYS_GET_TICKS       0u
#define SB_SYS_PROCESS_ID      1u
#define SB_SYS_EXIT            2u
#define SB_SYS_DISPLAY_INFO    3u
#define SB_SYS_DISPLAY_CLEAR   4u
#define SB_SYS_DISPLAY_RECT    5u
#define SB_SYS_INPUT_KEY       6u
#define SB_SYS_DISPLAY_GLYPH   7u
#define SB_SYS_INPUT_MOUSE     8u
#define SB_SYS_CONFIG_GET      9u
#define SB_SYS_CONFIG_SET      10u
#define SB_SYS_YIELD           11u
#define SB_SYS_DISPLAY_GLYPH_PAIR 12u
#define SB_CONFIG_SET_VOLATILE 1u

#ifdef SB_HOST_TEST
uint64_t sb_syscall0(uint64_t number);
uint64_t sb_syscall1(uint64_t number, uint64_t arg0);
uint64_t sb_syscall2(uint64_t number, uint64_t arg0, uint64_t arg1);
uint64_t sb_syscall3(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sb_syscall4(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t sb_get_ticks(void);
uint64_t sb_process_id(void);
uint64_t sb_display_info(void);
uint64_t sb_display_clear(uint32_t rgb);
uint64_t sb_display_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb);
uint64_t sb_input_key(void);
uint64_t sb_input_mouse(void);
uint64_t sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap, uint32_t rgb);
uint64_t sb_display_glyph_pair(uint32_t x, uint32_t y, uint64_t bitmap_a, uint64_t bitmap_b, uint32_t rgb);
uint64_t sb_config_get(void);
uint64_t sb_config_set(uint32_t language, uint32_t optional_enabled_mask);
uint64_t sb_yield(void);
#else
static inline uint64_t sb_syscall0(uint64_t number) {
    uint64_t result;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number) : "memory");
    return result;
}
static inline uint64_t sb_syscall1(uint64_t number, uint64_t arg0) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = arg0;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi) : "memory");
    return result;
}
static inline uint64_t sb_syscall2(uint64_t number, uint64_t arg0, uint64_t arg1) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = arg0;
    register uint64_t rsi __asm__("rsi") = arg1;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "memory");
    return result;
}
static inline uint64_t sb_syscall3(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = arg0;
    register uint64_t rsi __asm__("rsi") = arg1;
    register uint64_t rdx __asm__("rdx") = arg2;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx) : "memory");
    return result;
}
static inline uint64_t sb_syscall4(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = arg0;
    register uint64_t rsi __asm__("rsi") = arg1;
    register uint64_t rdx __asm__("rdx") = arg2;
    register uint64_t r10 __asm__("r10") = arg3;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10) : "memory");
    return result;
}
static inline uint64_t sb_get_ticks(void) { return sb_syscall0(SB_SYS_GET_TICKS); }
static inline uint64_t sb_process_id(void) { return sb_syscall0(SB_SYS_PROCESS_ID); }
static inline uint64_t sb_display_info(void) { return sb_syscall0(SB_SYS_DISPLAY_INFO); }
static inline uint64_t sb_display_clear(uint32_t rgb) { return sb_syscall1(SB_SYS_DISPLAY_CLEAR, rgb); }
static inline uint64_t sb_display_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = x;
    register uint64_t rsi __asm__("rsi") = y;
    register uint64_t rdx __asm__("rdx") = width;
    register uint64_t r10 __asm__("r10") = height;
    register uint64_t r8 __asm__("r8") = rgb;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(SB_SYS_DISPLAY_RECT), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8) : "memory");
    return result;
}
static inline uint64_t sb_input_key(void) { return sb_syscall0(SB_SYS_INPUT_KEY); }
static inline uint64_t sb_input_mouse(void) { return sb_syscall0(SB_SYS_INPUT_MOUSE); }
static inline uint64_t sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap, uint32_t rgb) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = x;
    register uint64_t rsi __asm__("rsi") = y;
    register uint64_t rdx __asm__("rdx") = bitmap;
    register uint64_t r10 __asm__("r10") = rgb;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(SB_SYS_DISPLAY_GLYPH), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10) : "memory");
    return result;
}
static inline uint64_t sb_display_glyph_pair(uint32_t x, uint32_t y, uint64_t bitmap_a, uint64_t bitmap_b, uint32_t rgb) {
    uint64_t result;
    register uint64_t rdi __asm__("rdi") = x;
    register uint64_t rsi __asm__("rsi") = y;
    register uint64_t rdx __asm__("rdx") = bitmap_a;
    register uint64_t r10 __asm__("r10") = bitmap_b;
    register uint64_t r8 __asm__("r8") = rgb;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(SB_SYS_DISPLAY_GLYPH_PAIR), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8) : "memory");
    return result;
}
static inline uint64_t sb_config_get(void) { return sb_syscall0(SB_SYS_CONFIG_GET); }
static inline uint64_t sb_config_set(uint32_t language, uint32_t optional_enabled_mask) {
    return sb_syscall2(SB_SYS_CONFIG_SET, language, optional_enabled_mask);
}
static inline uint64_t sb_yield(void) { return sb_syscall0(SB_SYS_YIELD); }
#endif

#endif
