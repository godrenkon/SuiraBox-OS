#include <assert.h>
#include <stdint.h>
#include "../kernel/process_exec.h"
#include "../kernel/arch/x86_64/irq_frame.h"

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
    assert(thread->kernel_resume_stack_pointer != 0u);
    assert(thread->kernel_resume_stack_pointer ==
           thread->kernel_stack_top - SB_USER_RESUME_FRAME_OFFSET);
    assert(thread->kernel_resume_stack_pointer + SB_USER_RESUME_FRAME_SIZE <=
           thread->kernel_stack_top - SB_USER_RESUME_FRAME_OFFSET + SB_USER_RESUME_FRAME_SIZE);
    assert(thread->kernel_resume_stack_pointer + SB_USER_RESUME_FRAME_SIZE <= thread->kernel_stack_top - 160u);

    const sb_timer_saved_gpr_t *gpr =
        (const sb_timer_saved_gpr_t *)(uintptr_t)thread->kernel_resume_stack_pointer;
    const sb_x86_64_user_iret_frame_t *iret =
        (const sb_x86_64_user_iret_frame_t *)(uintptr_t)(thread->kernel_resume_stack_pointer + sizeof(*gpr));
    assert(gpr->r15 == context.r15);
    assert(gpr->r14 == context.r14);
    assert(gpr->r13 == context.r13);
    assert(gpr->r12 == context.r12);
    assert(gpr->rbp == context.rbp);
    assert(gpr->rbx == context.rbx);
    assert(gpr->r11 == context.r11);
    assert(gpr->r10 == context.r10);
    assert(gpr->r9 == context.r9);
    assert(gpr->r8 == context.r8);
    assert(gpr->rdi == context.rdi);
    assert(gpr->rsi == context.rsi);
    assert(gpr->rdx == context.rdx);
    assert(gpr->rcx == context.rcx);
    assert(gpr->rax == context.rax);
    assert(iret->rip == context.rip);
    assert(iret->cs == context.cs);
    assert(iret->rflags == context.rflags);
    assert(iret->rsp == context.rsp);
    assert(iret->ss == context.ss);

    assert(process_prepare_elf_thread(&process, 1u, 128u, &context, &image, &thread) != 0);
    assert(thread == 0);

    image.entry_point = SB_USER_LIMIT;
    assert(process_prepare_elf_thread(&process, 2u, 128u, &context, &image, &thread) != 0);
    assert(thread == 0);
    assert(process.thread_count == 1u);
    return 0;
}
