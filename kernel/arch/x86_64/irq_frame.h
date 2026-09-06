#ifndef SB_ARCH_X86_64_IRQ_FRAME_H
#define SB_ARCH_X86_64_IRQ_FRAME_H

#include <stdint.h>

/*
 * Register image produced by the x86_64 IRQ entry stub.
 *
 * The first 15 fields are software-saved GPRs. rip/cs/rflags are always
 * supplied by the CPU. rsp/ss are supplied by the CPU only when an interrupt
 * crosses privilege levels (for example ring3 -> ring0); they are also present
 * in synthetic first-user-thread frames created by the scheduler.
 */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
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
} sb_irq_frame_t;

_Static_assert(sizeof(sb_irq_frame_t) == 20u * sizeof(uint64_t),
               "x86_64 IRQ frame layout mismatch");

static inline int sb_irq_frame_is_user(const sb_irq_frame_t *frame) {
    return frame != 0 && (frame->cs & 3u) == 3u;
}

#endif /* SB_ARCH_X86_64_IRQ_FRAME_H */
