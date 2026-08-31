#include "syscall.h"
#include "timer.h"
#include "scheduler.h"
#include "user_scheduler.h"
#include "framebuffer.h"
#include "storage.h"
#include "config_store.h"
#include "input.h"
#include "app_manager.h"
#include "fs_syscall.h"
#include "process.h"
#include "mm/address_space.h"

#define SB_SYSCALL_EXIT_SWITCH (UINT64_MAX - 1u)

static uint8_t syscall_user_smoke_seen;
static uint8_t syscall_user_draw_seen;

static void syscall_user_smoke_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if ((status & 0x20u) != 0u) break;
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void syscall_user_smoke_mark(void) {
    if (syscall_user_smoke_seen != 0u) return;
    syscall_user_smoke_seen = 1u;
    static const char message[] = "Userspace: ring3 syscall reached\r\n";
    for (uint32_t i = 0u; message[i] != '\0'; ++i) syscall_user_smoke_char(message[i]);
}

static void syscall_user_draw_mark(void) {
    if (syscall_user_draw_seen != 0u) return;
    syscall_user_draw_seen = 1u;
    static const char message[] = "Userspace: GUI framebuffer draw reached\r\n";
    for (uint32_t i = 0u; message[i] != '\0'; ++i) syscall_user_smoke_char(message[i]);
}

static uint64_t syscall_process_id(void) {
    sb_process_t *process = user_scheduler_current_process();
    if (process != 0) return process->pid;
    sb_task_t *task = scheduler_current();
    return task != 0 ? task->id : 0u;
}

static uint64_t syscall_display_info(void) {
    const sb_framebuffer_info_t *fb;
    syscall_user_smoke_mark();
    if (!sb_framebuffer_available()) return 0u;
    fb = sb_framebuffer_info();
    if (fb == 0 || fb->height > UINT16_MAX) return 0u;
    return ((uint64_t)fb->width << 32) | ((uint64_t)fb->height << 16) |
           ((uint64_t)fb->bits_per_pixel << 8) | 1u;
}

static uint64_t syscall_config_get(void) {
    sb_config_store_record_t record;
    if (!sb_config_store_get(&record)) return 0u;
    return 1u | ((uint64_t)record.language << 8) |
           ((uint64_t)record.optional_enabled_mask << 16);
}

static int syscall_exit_current(uint64_t exit_code) {
    sb_process_t *process = user_scheduler_current_process();
    sb_thread_t *thread = user_scheduler_current_thread();
    if (process == 0 || thread == 0) return -1;
    if (user_scheduler_request_exit(process, thread) != 0) return -1;
    thread->state = SB_PROCESS_EXITED;
    thread->runtime_ticks = 0u;
    uint32_t runnable = 0u;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        const sb_thread_t *candidate = &process->threads[i];
        if (candidate != thread &&
            (candidate->state == SB_PROCESS_CREATED || candidate->state == SB_PROCESS_RUNNING))
            ++runnable;
    }
    if (runnable == 0u) {
        process->state = SB_PROCESS_EXITED;
        process->exit_code = exit_code;
    }
    return 0;
}

static uint64_t syscall_wait_child(uint64_t child_pid, uint64_t user_exit_code) {
    sb_process_t *parent = user_scheduler_current_process();
    uint64_t exit_code = 0u;
    uint64_t result;
    if (parent == 0) return UINT64_MAX;
    if (user_exit_code != 0u && address_space_validate_user_range(&parent->address_space,
                                                                  user_exit_code,
                                                                  sizeof(uint64_t), 1u) != 0)
        return UINT64_MAX;
    result = process_wait_child(parent, child_pid, &exit_code);
    if (result == 0u || result == UINT64_MAX) return result;
    if (user_exit_code != 0u) *(uint64_t *)(uintptr_t)user_exit_code = exit_code;
    return result;
}

static void syscall_idle(void) {
    __asm__ volatile ("sti\n\thlt\n\tcli" ::: "memory");
}

uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                          uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    switch (number) {
        case SB_SYS_GET_TICKS: return timer_ticks();
        case SB_SYS_PROCESS_ID: return syscall_process_id();
        case SB_SYS_EXIT: return syscall_exit_current(arg0) == 0 ? SB_SYSCALL_EXIT_SWITCH : UINT64_MAX;
        case SB_SYS_DISPLAY_INFO: return syscall_display_info();
        case SB_SYS_DISPLAY_CLEAR:
            syscall_user_draw_mark();
            return sb_framebuffer_clear((uint8_t)((arg0 >> 16) & 0xFFu), (uint8_t)((arg0 >> 8) & 0xFFu), (uint8_t)(arg0 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_DISPLAY_RECT:
            syscall_user_draw_mark();
            if (arg0 > UINT32_MAX || arg1 > UINT32_MAX || arg2 > UINT32_MAX || arg3 > UINT32_MAX || arg4 > UINT32_MAX) return UINT64_MAX;
            return sb_framebuffer_fill_rect((uint32_t)arg0, (uint32_t)arg1, (uint32_t)arg2, (uint32_t)arg3,
                                            (uint8_t)((arg4 >> 16) & 0xFFu), (uint8_t)((arg4 >> 8) & 0xFFu), (uint8_t)(arg4 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_INPUT_KEY: return sb_input_read_key();
        case SB_SYS_DISPLAY_GLYPH:
            syscall_user_draw_mark();
            if (arg0 > UINT32_MAX - 7u || arg1 > UINT32_MAX - 7u || arg3 > UINT32_MAX) return UINT64_MAX;
            return sb_framebuffer_draw_glyph8((uint32_t)arg0, (uint32_t)arg1, arg2,
                                              (uint8_t)((arg3 >> 16) & 0xFFu), (uint8_t)((arg3 >> 8) & 0xFFu), (uint8_t)(arg3 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_DISPLAY_GLYPH_PAIR:
            syscall_user_draw_mark();
            if (arg0 > UINT32_MAX - 17u || arg1 > UINT32_MAX - 7u || arg4 > UINT32_MAX) return UINT64_MAX;
            if (sb_framebuffer_draw_glyph8((uint32_t)arg0, (uint32_t)arg1, arg2,
                                           (uint8_t)((arg4 >> 16) & 0xFFu), (uint8_t)((arg4 >> 8) & 0xFFu), (uint8_t)(arg4 & 0xFFu)) != 0) return UINT64_MAX;
            return sb_framebuffer_draw_glyph8((uint32_t)arg0 + 10u, (uint32_t)arg1, arg3,
                                              (uint8_t)((arg4 >> 16) & 0xFFu), (uint8_t)((arg4 >> 8) & 0xFFu), (uint8_t)(arg4 & 0xFFu)) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_INPUT_MOUSE: syscall_user_smoke_mark(); return sb_input_read_mouse();
        case SB_SYS_CONFIG_GET: return syscall_config_get();
        case SB_SYS_CONFIG_SET: {
            const uint32_t language = (uint32_t)arg0;
            const uint32_t optional_enabled_mask = (uint32_t)arg1;
            if (language > 3u ||
                (optional_enabled_mask != SB_CONFIG_SET_KEEP_OPTIONS &&
                 optional_enabled_mask > SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED)) return UINT64_MAX;
            if (!sb_storage_ready()) return UINT64_MAX;
            return sb_config_store_set((uint8_t)language, optional_enabled_mask) == 0 ? 0u : UINT64_MAX;
        }
        case SB_SYS_YIELD: syscall_idle(); return 0u;
        case SB_SYS_APP_LAUNCH:
            if (arg0 > UINT32_MAX) return UINT64_MAX;
            return sb_app_launch((uint32_t)arg0) == 0 ? 0u : UINT64_MAX;
        case SB_SYS_FS_LIST_ROOT:
        case SB_SYS_FS_STAT_ROOT:
        case SB_SYS_FS_READ_ROOT:
        case SB_SYS_FS_CREATE_ROOT:
        case SB_SYS_FS_WRITE_ROOT:
        case SB_SYS_FS_OPEN:
        case SB_SYS_FS_READ:
        case SB_SYS_FS_WRITE:
        case SB_SYS_FS_CLOSE:
            return sb_fs_syscall_dispatch(number, arg0, arg1, arg2, arg3, arg4);
        case SB_SYS_WAIT_CHILD:
            return syscall_wait_child(arg0, arg1);
        default: return UINT64_MAX;
    }
}

uint64_t sb_syscall_dispatch_entry(uint64_t number, uint64_t arg0, uint64_t arg1,
                                   uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    return syscall_dispatch(number, arg0, arg1, arg2, arg3, arg4);
}

void syscall_init(void) {
    (void)sb_storage_init();
    sb_input_init();
}
