#include "irq_frame.h"

int sb_user_context_from_timer_frame(sb_user_context_t *context,
                                     const sb_timer_saved_gpr_t *gpr,
                                     const sb_x86_64_user_iret_frame_t *iret) {
    if (context == 0 || gpr == 0 || iret == 0) return -1;

    sb_user_context_t next = {
        .r15 = gpr->r15,
        .r14 = gpr->r14,
        .r13 = gpr->r13,
        .r12 = gpr->r12,
        .r11 = gpr->r11,
        .r10 = gpr->r10,
        .r9 = gpr->r9,
        .r8 = gpr->r8,
        .rsi = gpr->rsi,
        .rdi = gpr->rdi,
        .rbp = gpr->rbp,
        .rdx = gpr->rdx,
        .rcx = gpr->rcx,
        .rbx = gpr->rbx,
        .rax = gpr->rax,
        .rip = iret->rip,
        .cs = iret->cs,
        .rflags = iret->rflags,
        .rsp = iret->rsp,
        .ss = iret->ss
    };

    if (sb_user_context_validate(&next) != 0) return -1;
    *context = next;
    return 0;
}
