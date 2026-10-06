#ifndef SB_USER_SYSCALL_CLIENT_H
#define SB_USER_SYSCALL_CLIENT_H
#include <stdint.h>
static inline int64_t sb_user_call3(uint64_t number, uint64_t a, uint64_t b, uint64_t c) {
    int64_t result;
    __asm__ volatile("int $0x80" : "=a"(result) : "0"(number), "D"(a), "S"(b), "d"(c) : "memory", "cc");
    return result;
}
#endif
