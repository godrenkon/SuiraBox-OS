#include <assert.h>
#include <stdint.h>
#include "../kernel/process.h"

static uint8_t stack_page[SB_USER_KERNEL_STACK_SIZE];
static uint32_t alloc_count;
static uint32_t free_count;

void *pmm_alloc_page(void) {
    ++alloc_count;
    return stack_page;
}

void pmm_free_page(void *page) {
    assert(page == stack_page);
    ++free_count;
}

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

int gdt_try_set_kernel_stack(uint64_t stack_pointer) {
    return stack_pointer != 0u ? 0 : -1;
}

int main(void) {
    sb_process_t process = {0};
    sb_user_context_t context = {0};
    sb_thread_t *thread;

    process.state = SB_PROCESS_CREATED;
    thread = process_create_thread(&process, 1u, 128u);
    assert(thread != 0);
    assert(thread->user_context == 0);
    assert(thread->kernel_stack_base == (uint64_t)(uintptr_t)stack_page);
    assert(thread->kernel_stack_top == (uint64_t)(uintptr_t)stack_page + SB_USER_KERNEL_STACK_SIZE);
    assert(alloc_count == 1u);

    assert(process_prepare_thread_context(thread, &context, SB_USER_BASE, SB_USER_STACK_TOP) == 0);
    assert(thread->user_context == &context);
    assert(sb_user_context_validate(&context) == 0);

    assert(process_prepare_thread_context(thread, &context, SB_USER_BASE, SB_USER_STACK_TOP - 1u) != 0);
    assert(thread->user_context == &context);
    assert(sb_user_context_validate(&context) == 0);

    assert(process_create_thread(&process, 1u, 128u) != 0);
    assert(process_create_thread(&process, 2u, 128u) != 0);
    process_destroy(&process);
    assert(free_count == 2u);
    return 0;
}
