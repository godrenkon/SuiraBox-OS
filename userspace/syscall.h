#ifndef SB_USER_SYSCALL_H
#define SB_USER_SYSCALL_H

#include <stdint.h>

#define SB_SYS_GET_TICKS  0u
#define SB_SYS_PROCESS_ID 1u
#define SB_SYS_EXIT       2u

static inline uint64_t sb_syscall0(uint64_t number) {
    uint64_t result;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number) : "memory");
    return result;
}

static inline uint64_t sb_get_ticks(void) {
    return sb_syscall0(SB_SYS_GET_TICKS);
}

static inline uint64_t sb_process_id(void) {
    return sb_syscall0(SB_SYS_PROCESS_ID);
}

#endif
