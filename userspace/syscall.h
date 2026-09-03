#ifndef SB_USER_SYSCALL_H
#define SB_USER_SYSCALL_H

#include <stdint.h>
#include "../include/sb_fs_abi.h"

#define SB_SYSCALL_ABI_MAJOR   1u
#define SB_SYSCALL_ABI_MINOR   2u
#define SB_SYS_GET_TICKS          0u
#define SB_SYS_PROCESS_ID         1u
#define SB_SYS_EXIT               2u
#define SB_SYS_DISPLAY_INFO       3u
#define SB_SYS_DISPLAY_CLEAR      4u
#define SB_SYS_DISPLAY_RECT       5u
#define SB_SYS_INPUT_KEY          6u
#define SB_SYS_DISPLAY_GLYPH      7u
#define SB_SYS_INPUT_MOUSE        8u
#define SB_SYS_CONFIG_GET         9u
#define SB_SYS_CONFIG_SET         10u
#define SB_SYS_YIELD              11u
#define SB_SYS_DISPLAY_GLYPH_PAIR 12u
#define SB_SYS_APP_LAUNCH         13u
#define SB_SYS_FS_LIST_ROOT       14u
#define SB_SYS_FS_STAT_ROOT       15u
#define SB_SYS_FS_READ_ROOT       16u
#define SB_SYS_FS_CREATE_ROOT     17u
#define SB_SYS_FS_WRITE_ROOT      18u
#define SB_SYS_WAIT_CHILD         19u
#define SB_SYS_FS_OPEN            20u
#define SB_SYS_FS_READ            21u
#define SB_SYS_FS_WRITE           22u
#define SB_SYS_FS_CLOSE           23u
#define SB_SYS_SLEEP              24u
#define SB_SYS_ABI_VERSION        25u
#define SB_SYS_FS_SEEK             26u
#define SB_SYS_FS_LIST             27u

#define SB_FS_OPEN_READ   0x01u
#define SB_FS_OPEN_WRITE  0x02u
#define SB_FS_OPEN_CREATE 0x04u
#define SB_FS_SEEK_SET 0u
#define SB_FS_SEEK_CUR 1u
#define SB_FS_SEEK_END 2u
#define SB_CONFIG_SET_VOLATILE    1u
#define SB_CONFIG_SET_KEEP_OPTIONS 0xFFFFFFFFu

_Static_assert(SB_SYS_ABI_VERSION == 25u, "syscall ABI version query number changed");
_Static_assert(SB_SYSCALL_ABI_MAJOR == 1u, "syscall ABI major changed");
_Static_assert(SB_SYSCALL_ABI_MINOR == 2u, "syscall ABI minor changed");

#ifdef SB_HOST_TEST
uint64_t sb_syscall0(uint64_t number);
uint64_t sb_syscall1(uint64_t number, uint64_t arg0);
uint64_t sb_syscall2(uint64_t number, uint64_t arg0, uint64_t arg1);
uint64_t sb_syscall3(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sb_syscall4(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t sb_syscall5(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
uint64_t sb_get_ticks(void);
uint64_t sb_process_id(void);
uint64_t sb_syscall_abi_version(void);
uint64_t sb_display_info(void);
uint64_t sb_display_clear(uint32_t rgb);
uint64_t sb_display_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb);
uint64_t sb_input_key(void);
uint64_t sb_input_mouse(void);
uint64_t sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap, uint32_t rgb);
uint64_t sb_display_glyph_pair(uint32_t x, uint32_t y, uint64_t bitmap_a, uint64_t bitmap_b, uint32_t rgb);
uint64_t sb_config_get(void);
uint64_t sb_config_set_with_options(uint32_t language, uint32_t optional_enabled_mask);
uint64_t sb_app_launch(uint32_t app_id);
uint64_t sb_fs_list_root(char *buffer, uint32_t capacity);
uint64_t sb_fs_list(const char *path, uint32_t path_length, void *buffer, uint32_t capacity);
uint64_t sb_fs_stat_root(const char *name, uint32_t name_length);
uint64_t sb_fs_read_root(const char *name, uint32_t name_length, void *buffer, uint32_t capacity, uint32_t offset);
uint64_t sb_fs_create_root(const char *name, uint32_t name_length, uint32_t file_size);
uint64_t sb_fs_write_root(const char *name, uint32_t name_length, const void *buffer, uint32_t length, uint32_t offset);
uint64_t sb_fs_open(const char *path, uint32_t path_length, uint32_t flags, uint32_t initial_size);
uint64_t sb_fs_read(uint64_t fd, void *buffer, uint32_t length);
uint64_t sb_fs_write(uint64_t fd, const void *buffer, uint32_t length);
uint64_t sb_fs_close(uint64_t fd);
uint64_t sb_fs_seek(uint64_t fd, int64_t offset, uint32_t whence);
uint64_t sb_wait_child(uint64_t child_pid, uint64_t *exit_code);
uint64_t sb_sleep(uint64_t ticks);
static inline uint64_t sb_config_set(uint32_t language) { return sb_config_set_with_options(language, SB_CONFIG_SET_KEEP_OPTIONS); }
uint64_t sb_yield(void);
#else
static inline uint64_t sb_syscall0(uint64_t number) { uint64_t result; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number) : "memory"); return result; }
static inline uint64_t sb_syscall1(uint64_t number, uint64_t arg0) { uint64_t result; register uint64_t rdi __asm__("rdi") = arg0; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi) : "memory"); return result; }
static inline uint64_t sb_syscall2(uint64_t number, uint64_t arg0, uint64_t arg1) { uint64_t result; register uint64_t rdi __asm__("rdi") = arg0; register uint64_t rsi __asm__("rsi") = arg1; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi) : "memory"); return result; }
static inline uint64_t sb_syscall3(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2) { uint64_t result; register uint64_t rdi __asm__("rdi") = arg0; register uint64_t rsi __asm__("rsi") = arg1; register uint64_t rdx __asm__("rdx") = arg2; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx) : "memory"); return result; }
static inline uint64_t sb_syscall4(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) { uint64_t result; register uint64_t rdi __asm__("rdi") = arg0; register uint64_t rsi __asm__("rsi") = arg1; register uint64_t rdx __asm__("rdx") = arg2; register uint64_t r10 __asm__("r10") = arg3; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10) : "memory"); return result; }
static inline uint64_t sb_syscall5(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) { uint64_t result; register uint64_t rdi __asm__("rdi") = arg0; register uint64_t rsi __asm__("rsi") = arg1; register uint64_t rdx __asm__("rdx") = arg2; register uint64_t r10 __asm__("r10") = arg3; register uint64_t r8 __asm__("r8") = arg4; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10) : "memory"); return result; }
static inline uint64_t sb_get_ticks(void) { return sb_syscall0(SB_SYS_GET_TICKS); }
static inline uint64_t sb_process_id(void) { return sb_syscall0(SB_SYS_PROCESS_ID); }
static inline uint64_t sb_syscall_abi_version(void) { return sb_syscall0(SB_SYS_ABI_VERSION); }
static inline uint64_t sb_display_info(void) {
#ifdef SB_RUNTIME_SMOKE
    static uint8_t attempted;
    static uint8_t smoke_ok;
    if (attempted == 0u) {
        char path[] = "/SBRUN.TST";
        char write_data[] = "SBOK";
        char read_data[4] = {0};
        attempted = 1u;
        const uint64_t fd = sb_syscall4(SB_SYS_FS_OPEN, (uint64_t)(uintptr_t)path, 9u,
                                         SB_FS_OPEN_READ | SB_FS_OPEN_WRITE | SB_FS_OPEN_CREATE, 4u);
        if (fd != UINT64_MAX &&
            sb_syscall3(SB_SYS_FS_WRITE, fd, (uint64_t)(uintptr_t)write_data, 4u) == 4u &&
            sb_syscall3(SB_SYS_FS_SEEK, fd, 0u, SB_FS_SEEK_SET) == 0u &&
            sb_syscall3(SB_SYS_FS_READ, fd, (uint64_t)(uintptr_t)read_data, 4u) == 4u &&
            read_data[0] == 'S' && read_data[1] == 'B' && read_data[2] == 'O' && read_data[3] == 'K' &&
            sb_syscall1(SB_SYS_FS_CLOSE, fd) == 0u) {
            smoke_ok = 1u;
        }
        if (smoke_ok == 0u && fd != UINT64_MAX) (void)sb_syscall1(SB_SYS_FS_CLOSE, fd);
    }
    if (smoke_ok == 0u) return 0u;
#endif
    return sb_syscall0(SB_SYS_DISPLAY_INFO);
}
static inline uint64_t sb_display_clear(uint32_t rgb) { return sb_syscall1(SB_SYS_DISPLAY_CLEAR, rgb); }
static inline uint64_t sb_display_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb) { return sb_syscall5(SB_SYS_DISPLAY_RECT,x,y,width,height,rgb); }
static inline uint64_t sb_input_key(void) { return sb_syscall0(SB_SYS_INPUT_KEY); }
static inline uint64_t sb_input_mouse(void) { return sb_syscall0(SB_SYS_INPUT_MOUSE); }
static inline uint64_t sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap, uint32_t rgb) { return sb_syscall4(SB_SYS_DISPLAY_GLYPH,x,y,bitmap,rgb); }
static inline uint64_t sb_display_glyph_pair(uint32_t x,uint32_t y,uint64_t bitmap_a,uint64_t bitmap_b,uint32_t rgb){return sb_syscall5(SB_SYS_DISPLAY_GLYPH_PAIR,x,y,bitmap_a,bitmap_b,rgb);}
static inline uint64_t sb_config_get(void) { return sb_syscall0(SB_SYS_CONFIG_GET); }
static inline uint64_t sb_config_set_with_options(uint32_t language,uint32_t optional_enabled_mask){return sb_syscall2(SB_SYS_CONFIG_SET,language,optional_enabled_mask);}
static inline uint64_t sb_config_set(uint32_t language){return sb_config_set_with_options(language,SB_CONFIG_SET_KEEP_OPTIONS);}
static inline uint64_t sb_app_launch(uint32_t app_id){return sb_syscall1(SB_SYS_APP_LAUNCH,app_id);}
static inline uint64_t sb_fs_list_root(char *buffer,uint32_t capacity){return sb_syscall2(SB_SYS_FS_LIST_ROOT,(uint64_t)(uintptr_t)buffer,capacity);}
static inline uint64_t sb_fs_list(const char *path,uint32_t path_length,void *buffer,uint32_t capacity){return sb_syscall4(SB_SYS_FS_LIST,(uint64_t)(uintptr_t)path,path_length,(uint64_t)(uintptr_t)buffer,capacity);}
static inline uint64_t sb_fs_stat_root(const char *name,uint32_t name_length){return sb_syscall2(SB_SYS_FS_STAT_ROOT,(uint64_t)(uintptr_t)name,name_length);}
static inline uint64_t sb_fs_read_root(const char *name,uint32_t name_length,void *buffer,uint32_t capacity,uint32_t offset){return sb_syscall5(SB_SYS_FS_READ_ROOT,(uint64_t)(uintptr_t)name,name_length,(uint64_t)(uintptr_t)buffer,capacity,offset);}
static inline uint64_t sb_fs_create_root(const char *name,uint32_t name_length,uint32_t file_size){return sb_syscall3(SB_SYS_FS_CREATE_ROOT,(uint64_t)(uintptr_t)name,name_length,file_size);}
static inline uint64_t sb_fs_write_root(const char *name,uint32_t name_length,const void *buffer,uint32_t length,uint32_t offset){return sb_syscall5(SB_SYS_FS_WRITE_ROOT,(uint64_t)(uintptr_t)name,name_length,(uint64_t)(uintptr_t)buffer,length,offset);}
static inline uint64_t sb_fs_open(const char *path,uint32_t path_length,uint32_t flags,uint32_t initial_size){return sb_syscall4(SB_SYS_FS_OPEN,(uint64_t)(uintptr_t)path,path_length,flags,initial_size);}
static inline uint64_t sb_fs_read(uint64_t fd,void *buffer,uint32_t length){return sb_syscall3(SB_SYS_FS_READ,fd,(uint64_t)(uintptr_t)buffer,length);}
static inline uint64_t sb_fs_write(uint64_t fd,const void *buffer,uint32_t length){return sb_syscall3(SB_SYS_FS_WRITE,fd,(uint64_t)(uintptr_t)buffer,length);}
static inline uint64_t sb_fs_close(uint64_t fd){return sb_syscall1(SB_SYS_FS_CLOSE,fd);}
static inline uint64_t sb_fs_seek(uint64_t fd,int64_t offset,uint32_t whence){return sb_syscall3(SB_SYS_FS_SEEK,fd,(uint64_t)offset,whence);}
static inline uint64_t sb_wait_child(uint64_t child_pid,uint64_t *exit_code){return sb_syscall2(SB_SYS_WAIT_CHILD,child_pid,(uint64_t)(uintptr_t)exit_code);}
static inline uint64_t sb_sleep(uint64_t ticks){return sb_syscall1(SB_SYS_SLEEP,ticks);}
static inline uint64_t sb_yield(void){return sb_syscall0(SB_SYS_YIELD);}
#endif

#endif
