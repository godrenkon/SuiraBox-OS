#include <assert.h>
#include <stdint.h>
#include "../kernel/process.h"

static uint8_t pages[8][4096];
static uint8_t page_used[8];
static int rebind_calls;
static sb_thread_t *rebind_old;
static sb_thread_t *rebind_new;

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

void *pmm_alloc_page(void) {
    for (uint32_t i = 0u; i < 8u; ++i) {
        if (page_used[i] == 0u) {
            page_used[i] = 1u;
            return pages[i];
        }
    }
    return 0;
}

void pmm_free_page(void *page) {
    for (uint32_t i = 0u; i < 8u; ++i) {
        if ((void *)pages[i] == page) {
            page_used[i] = 0u;
            return;
        }
    }
}

int user_scheduler_remove(sb_process_t *process, sb_thread_t *thread) {
    (void)process;
    (void)thread;
    return 0;
}

int user_scheduler_rebind_thread(sb_process_t *process, sb_thread_t *old_thread, sb_thread_t *new_thread) {
    (void)process;
    rebind_old = old_thread;
    rebind_new = new_thread;
    ++rebind_calls;
    return 0;
}

int sb_user_context_init(sb_user_context_t *context, uint64_t entry_point, uint64_t user_stack_top) {
    (void)context;
    (void)entry_point;
    (void)user_stack_top;
    return 0;
}

int sb_user_context_validate(const sb_user_context_t *context) {
    return context != 0 ? 0 : -1;
}

int main(void) {
    sb_process_t process = {0};
    sb_thread_t *first;
    sb_thread_t *middle;
    sb_thread_t *last;

    process.state = SB_PROCESS_CREATED;
    first = process_create_thread(&process, 1u, 1u);
    middle = process_create_thread(&process, 2u, 2u);
    last = process_create_thread(&process, 3u, 3u);
    assert(first != 0 && middle != 0 && last != 0);
    assert(first->kernel_stack_base == (uint64_t)(uintptr_t)pages[0]);
    assert(middle->kernel_stack_base == (uint64_t)(uintptr_t)pages[1]);
    assert(last->kernel_stack_base == (uint64_t)(uintptr_t)pages[2]);
    assert(process.thread_count == 3u);

    rebind_calls = 0;
    assert(process_destroy_thread(&process, middle) == 0);
    assert(process.thread_count == 2u);
    assert(process.threads[0].tid == 1u);
    assert(process.threads[1].tid == 3u);
    assert(process.threads[1].kernel_stack_base == (uint64_t)(uintptr_t)pages[2]);
    assert(page_used[1] == 0u);
    assert(page_used[2] == 1u);
    assert(rebind_calls == 1);
    assert(rebind_old != 0 && rebind_new != 0);
    assert(rebind_old->tid == 3u);
    assert(rebind_new == &process.threads[1]);

    assert(process_destroy_thread(&process, &process.threads[0]) == 0);
    assert(process.thread_count == 1u);
    assert(process.threads[0].tid == 3u);
    assert(page_used[0] == 0u);
    assert(page_used[2] == 1u);

    assert(process_destroy_thread(&process, &process.threads[0]) == 0);
    assert(process.thread_count == 0u);
    assert(page_used[2] == 0u);
    assert(process_destroy_thread(&process, &process.threads[0]) != 0);
    return 0;
}
