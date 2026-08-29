#include "user_context.h"
#include "irq_frame.h"
#include "../../mm/address_space.h"

#define SB_USER_RFLAGS_IOPL_MASK (3ull << 12u)
#define SB_USER_RFLAGS_NT        (1ull << 14u)
#define SB_USER_RFLAGS_VM        (1ull << 17u)
#define SB_USER_RFLAGS_VIF       (1ull << 19u)
#define SB_USER_RFLAGS_VIP       (1ull << 20u)

static int user_address_valid(uint64_t address) {
    return address >= SB_USER_BASE && address < SB_USER_LIMIT &&
           (address >> 63u) == 0u;
}

int sb_user_context_init(sb_user_context_t *context,
                         uint64_t entry_point,
                         uint64_t user_stack_top) {
    if (context == 0 || !user_address_valid(entry_point) ||
        !user_address_valid(user_stack_top) ||
        (user_stack_top & (SB_USER_RSP_ALIGNMENT - 1u)) != 0u) return -1;

    *context = (sb_user_context_t){0};
    context->rip = entry_point;
    context->cs = SB_USER_CODE_SELECTOR;
    context->rflags = SB_USER_RFLAGS_RESERVED | SB_USER_RFLAGS_INTERRUPT;
    context->rsp = user_stack_top;
    context->ss = SB_USER_DATA_SELECTOR;
    return sb_user_context_validate(context);
}

int sb_user_context_validate(const sb_user_context_t *context) {
    if (context == 0 || !user_address_valid(context->rip) ||
        !user_address_valid(context->rsp) ||
        context->cs != SB_USER_CODE_SELECTOR ||
        context->ss != SB_USER_DATA_SELECTOR ||
        (context->rsp & (SB_USER_RSP_ALIGNMENT - 1u)) != 0u ||
        (context->rflags & SB_USER_RFLAGS_RESERVED) == 0u ||
        (context->rflags & SB_USER_RFLAGS_INTERRUPT) == 0u ||
        (context->rflags & SB_USER_RFLAGS_IOPL_MASK) != 0u ||
        (context->rflags & SB_USER_RFLAGS_NT) != 0u ||
        (context->rflags & SB_USER_RFLAGS_VM) != 0u ||
        (context->rflags & SB_USER_RFLAGS_VIF) != 0u ||
        (context->rflags & SB_USER_RFLAGS_VIP) != 0u) return -1;
    return 0;
}

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
