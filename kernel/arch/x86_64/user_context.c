#include "user_context.h"
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
    uint64_t initial_rsp;
    if (context == 0 || !user_address_valid(entry_point) ||
        !user_address_valid(user_stack_top) ||
        user_stack_top < SB_USER_BASE + sizeof(uint64_t) ||
        (user_stack_top & (SB_USER_RSP_ALIGNMENT - 1u)) != 0u) return -1;

    initial_rsp = user_stack_top;

    *context = (sb_user_context_t){0};
    context->rip = entry_point;
    context->cs = SB_USER_CODE_SELECTOR;
    context->rflags = SB_USER_RFLAGS_RESERVED | SB_USER_RFLAGS_INTERRUPT;
    context->rsp = initial_rsp;
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
