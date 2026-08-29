#ifndef SB_ARCH_X86_64_USER_CONTEXT_H
#define SB_ARCH_X86_64_USER_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
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

_Static_assert(sizeof(sb_user_context_t) == 160u, "user context ABI size changed");
_Static_assert(offsetof(sb_user_context_t, r15) == 0u, "user context ABI r15 offset changed");
_Static_assert(offsetof(sb_user_context_t, r8) == 56u, "user context ABI r8 offset changed");
_Static_assert(offsetof(sb_user_context_t, rsi) == 64u, "user context ABI rsi offset changed");
_Static_assert(offsetof(sb_user_context_t, rdi) == 72u, "user context ABI rdi offset changed");
_Static_assert(offsetof(sb_user_context_t, rbp) == 80u, "user context ABI rbp offset changed");
_Static_assert(offsetof(sb_user_context_t, rdx) == 88u, "user context ABI rdx offset changed");
_Static_assert(offsetof(sb_user_context_t, rcx) == 96u, "user context ABI rcx offset changed");
_Static_assert(offsetof(sb_user_context_t, rbx) == 104u, "user context ABI rbx offset changed");
_Static_assert(offsetof(sb_user_context_t, rax) == 112u, "user context ABI rax offset changed");
_Static_assert(offsetof(sb_user_context_t, rip) == 120u, "user context ABI rip offset changed");
_Static_assert(offsetof(sb_user_context_t, cs) == 128u, "user context ABI cs offset changed");
_Static_assert(offsetof(sb_user_context_t, rflags) == 136u, "user context ABI rflags offset changed");
_Static_assert(offsetof(sb_user_context_t, rsp) == 144u, "user context ABI rsp offset changed");
_Static_assert(offsetof(sb_user_context_t, ss) == 152u, "user context ABI ss offset changed");

int sb_user_context_init(sb_user_context_t *context,
                         uint64_t entry_point,
                         uint64_t user_stack_top);
int sb_user_context_validate(const sb_user_context_t *context);

#endif
