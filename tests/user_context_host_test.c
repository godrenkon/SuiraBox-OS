#include <assert.h>
#include <stdint.h>
#include "../kernel/arch/x86_64/user_context.h"
#include "../kernel/mm/address_space.h"

int main(void) {
    sb_user_context_t context;

    assert(sb_user_context_init(&context, SB_USER_BASE, SB_USER_STACK_TOP) == 0);
    assert(context.rip == SB_USER_BASE);
    assert(context.rsp == SB_USER_STACK_TOP - sizeof(uint64_t));
    assert((context.rsp & 0xFu) == 8u);
    assert(context.cs == SB_USER_CODE_SELECTOR);
    assert(context.ss == SB_USER_DATA_SELECTOR);
    assert(context.rflags == (SB_USER_RFLAGS_RESERVED | SB_USER_RFLAGS_INTERRUPT));
    assert(sb_user_context_validate(&context) == 0);

    context.rsp = SB_USER_STACK_TOP - 0x10u;
    assert((context.rsp & 0xFu) == 0u);
    assert(sb_user_context_validate(&context) == 0);
    context.rsp = SB_USER_STACK_TOP - 0x18u;
    assert((context.rsp & 0xFu) == 8u);
    assert(sb_user_context_validate(&context) == 0);

    context.rflags &= ~SB_USER_RFLAGS_RESERVED;
    assert(sb_user_context_validate(&context) != 0);
    context.rflags |= SB_USER_RFLAGS_RESERVED;
    context.rflags |= (1ull << 12u);
    assert(sb_user_context_validate(&context) != 0);
    context.rflags &= ~(3ull << 12u);
    context.rflags |= (1ull << 17u);
    assert(sb_user_context_validate(&context) != 0);
    context.rflags &= ~(1ull << 17u);
    context.rflags |= (1ull << 14u);
    assert(sb_user_context_validate(&context) != 0);
    context.rflags &= ~(1ull << 14u);
    context.rflags |= (1ull << 19u);
    assert(sb_user_context_validate(&context) != 0);
    context.rflags &= ~(1ull << 19u);
    context.rflags |= (1ull << 20u);
    assert(sb_user_context_validate(&context) != 0;
    context.rflags &= ~(1ull << 20u);
    context.cs = SB_USER_CODE_SELECTOR;
    assert(sb_user_context_validate(&context) == 0);
    context.cs = 0x1Bu;
    assert(sb_user_context_validate(&context) != 0);

    assert(sb_user_context_init(&context, SB_USER_BASE - 1u, SB_USER_STACK_TOP) != 0);
    assert(sb_user_context_init(&context, SB_USER_LIMIT, SB_USER_STACK_TOP) != 0);
    assert(sb_user_context_init(&context, SB_USER_BASE, SB_USER_STACK_TOP - 1u) != 0);
    return 0;
}
