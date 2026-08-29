#ifndef SB_ARCH_X86_64_IRQ_FRAME_H
#define SB_ARCH_X86_64_IRQ_FRAME_H

#include <stdint.h>
#include <stddef.h>

/* Layout at %rsp immediately after sb_timer_irq_stub saves all GPRs. */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
} sb_timer_saved_gpr_t;

_Static_assert(sizeof(sb_timer_saved_gpr_t) == 120u, "timer saved GPR frame size changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r15) == 0u, "timer GPR ABI r15 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r14) == 8u, "timer GPR ABI r14 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r13) == 16u, "timer GPR ABI r13 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r12) == 24u, "timer GPR ABI r12 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rbp) == 32u, "timer GPR ABI rbp offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rbx) == 40u, "timer GPR ABI rbx offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r11) == 48u, "timer GPR ABI r11 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r10) == 56u, "timer GPR ABI r10 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r9) == 64u, "timer GPR ABI r9 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, r8) == 72u, "timer GPR ABI r8 offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rdi) == 80u, "timer GPR ABI rdi offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rsi) == 88u, "timer GPR ABI rsi offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rdx) == 96u, "timer GPR ABI rdx offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rcx) == 104u, "timer GPR ABI rcx offset changed");
_Static_assert(offsetof(sb_timer_saved_gpr_t, rax) == 112u, "timer GPR ABI rax offset changed");

#endif
