#include <assert.h>
#include <stdint.h>
#include "../kernel/process.h"

int address_space_create(sb_address_space_t *space) {
    if (space == 0) return -1;
    space->pml4_physical = 1u;
    return 0;
}

void address_space_destroy(sb_address_space_t *space) {
    if (space != 0) space->pml4_physical = 0u;
}

int address_space_activate(const sb_address_space_t *space) {
    return space != 0 && space->pml4_physical != 0u ? 0 : -1;
}

int main(void) {
    sb_process_t process = {0};
    sb_user_context_t context = {0};
    sb_thread_t *thread;

    process.state = SB_PROCESS_CREATED;
    thread = process_create_thread(&process, 1u, 128u);
    assert(thread != 0);
    assert(thread->user_context == 0);

    assert(process_prepare_thread_context(thread, &context, SB_USER_BASE, SB_USER_STACK_TOP) == 0);
    assert(thread->user_context == &context);
    assert(sb_user_context_validate(&context) == 0);

    assert(process_prepare_thread_context(thread, &context, SB_USER_BASE, SB_USER_STACK_TOP - 1u) != 0);
    assert(thread->user_context == &context);
    assert(sb_user_context_validate(&context) == 0);

    assert(process_create_thread(&process, 1u, 128u) != 0);
    return 0;
}
