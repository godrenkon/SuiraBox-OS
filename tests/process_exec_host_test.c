#include <assert.h>
#include <stdint.h>
#include "../kernel/process_exec.h"

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

int address_space_map_user(sb_address_space_t *space, uint64_t virtual_address,
                           uint64_t physical_address, uint64_t flags) {
    (void)space; (void)virtual_address; (void)physical_address; (void)flags;
    return 0;
}

void *pmm_alloc_page(void) { static uint8_t page[4096]; return page; }
void pmm_free_page(void *page) { (void)page; }

int elf64_load_image(sb_address_space_t *space, const void *image,
                     uint64_t image_size, uint64_t *entry_point) {
    (void)space; (void)image; (void)image_size;
    if (entry_point == 0) return -1;
    *entry_point = SB_USER_BASE;
    return 0;
}

int multiboot_find_module(uint64_t info_addr, const char *name, sb_multiboot_module_t *module) {
    (void)info_addr; (void)name; (void)module;
    return -1;
}

int main(void) {
    sb_process_t process = {0};
    sb_process_image_t image = { .entry_point = SB_USER_BASE,
                                 .user_stack_top = SB_USER_STACK_TOP,
                                 .user_stack_bottom = SB_USER_STACK_TOP - 16384u };
    sb_user_context_t context = {0};
    sb_thread_t *thread = 0;

    process.state = SB_PROCESS_CREATED;
    assert(process_prepare_elf_thread(&process, 1u, 128u, &context, &image, &thread) == 0);
    assert(thread != 0);
    assert(thread->user_context == &context);
    assert(context.rip == SB_USER_BASE);
    assert(context.rsp == SB_USER_STACK_TOP);

    assert(process_prepare_elf_thread(&process, 1u, 128u, &context, &image, &thread) != 0);
    assert(thread == 0);

    image.entry_point = SB_USER_LIMIT;
    assert(process_prepare_elf_thread(&process, 2u, 128u, &context, &image, &thread) != 0);
    assert(thread == 0);
    assert(process.thread_count == 1u);
    return 0;
}
