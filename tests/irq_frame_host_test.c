#include <assert.h>
#include "../kernel/arch/x86_64/irq_frame.h"
#include "../kernel/mm/address_space.h"

int main(void) {
    sb_timer_saved_gpr_t gpr = {
        .r15 = 0x1500u, .r14 = 0x1400u, .r13 = 0x1300u, .r12 = 0x1200u,
        .rbp = 0x0B00u, .rbx = 0x0A00u, .r11 = 0x1100u, .r10 = 0x1000u,
        .r9 = 0x0900u, .r8 = 0x0800u, .rdi = 0x0700u, .rsi = 0x0600u,
        .rdx = 0x0500u, .rcx = 0x0400u, .rax = 0x0100u
    };
    sb_x86_64_user_iret_frame_t iret = {
        .rip = 0x00400000u,
        .cs = SB_USER_CODE_SELECTOR,
        .rflags = SB_USER_RFLAGS_RESERVED | SB_USER_RFLAGS_INTERRUPT,
        .rsp = SB_USER_STACK_TOP,
        .ss = SB_USER_DATA_SELECTOR
    };
    sb_user_context_t context;

    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) == 0);
    assert(context.r15 == gpr.r15);
    assert(context.r14 == gpr.r14);
    assert(context.r13 == gpr.r13);
    assert(context.r12 == gpr.r12);
    assert(context.r11 == gpr.r11);
    assert(context.r10 == gpr.r10);
    assert(context.r9 == gpr.r9);
    assert(context.r8 == gpr.r8);
    assert(context.rdi == gpr.rdi);
    assert(context.rsi == gpr.rsi);
    assert(context.rdx == gpr.rdx);
    assert(context.rcx == gpr.rcx);
    assert(context.rbx == gpr.rbx);
    assert(context.rbp == gpr.rbp);
    assert(context.rax == gpr.rax);
    assert(context.rip == iret.rip);
    assert(context.cs == iret.cs);
    assert(context.rflags == iret.rflags);
    assert(context.rsp == iret.rsp);
    assert(context.ss == iret.ss);

    iret.cs = SB_KERNEL_CODE_SELECTOR;
    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) != 0);
    iret.cs = SB_USER_CODE_SELECTOR;
    iret.rsp = SB_USER_STACK_TOP - 1u;
    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) != 0);
    iret.rsp = SB_USER_STACK_TOP;
    iret.rflags &= ~SB_USER_RFLAGS_INTERRUPT;
    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) != 0);

    assert(sb_user_context_from_timer_frame(0, &gpr, &iret) != 0);
    assert(sb_user_context_from_timer_frame(&context, 0, &iret) != 0);
    assert(sb_user_context_from_timer_frame(&context, &gpr, 0) != 0);
    return 0;
}
