#ifndef SB_ARCH_X86_64_USER_CONTEXT_H
#define SB_ARCH_X86_64_USER_CONTEXT_H

#include <stdint.h>
#include "gdt.h"

#define SB_USER_RFLAGS_RESERVED 0x2u
#define SB_USER_RFLAGS_INTERRUPT 0x200u
#define SB_USER_RSP_ALIGNMENT 16u

/* Register state required when a userspace thread is resumed. */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} sb_user_context_t;

int sb_user_context_init(sb_user_context_t *context,
                         uint64_t entry_point,
                         uint64_t user_stack_top);
int sb_user_context_validate(const sb_user_context_t *context);

#endif
