#include <assert.h>
#include "../kernel/arch/x86_64/irq_frame.h"
#include "../kernel/mm/address_space.h"

int main(void) {
    sb_timer_saved_gpr_t gpr = {0};
    sb_x86_64_user_iret_frame_t iret = {0};
    sb_user_context_t context;

    gpr.r15 = 15u;
    gpr.r14 = 14u;
    gpr.r13 = 13u;
    gpr.r12 = 12u;
    gpr.rbp = 11u;
    gpr.rbx = 10u;
    gpr.r11 = 9u;
    gpr.r10 = 8u;
    gpr.r9 = 7u;
    gpr.r8 = 6u;
    gpr.rdi = 5u;
    gpr.rsi = 4u;
    gpr.rdx = 3u;
    gpr.rcx = 2u;
    gpr.rax = 1u;
    iret.rip = SB_USER_BASE;
    iret.cs = SB_USER_CODE_SELECTOR;
    iret.rflags = SB_USER_RFLAGS_RESERVED | SB_USER_RFLAGS_INTERRUPT;
    iret.rsp = SB_USER_STACK_TOP;
    iret.ss = SB_USER_DATA_SELECTOR;

    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) == 0);
    assert(context.r15 == 15u);
    assert(context.rax == 1u);
    assert(context.rip == SB_USER_BASE);
    assert(context.rsp == SB_USER_STACK_TOP);

    iret.cs = SB_KERNEL_CODE_SELECTOR;
    assert(sb_user_context_from_timer_frame(&context, &gpr, &iret) != 0);
    return 0;
}
