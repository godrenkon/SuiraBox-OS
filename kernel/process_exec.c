#include "process_exec.h"
#include "elf_loader.h"
#include "mm/address_space.h"
#include "mm/multiboot_modules.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define SB_USER_STACK_PAGES 4u

static int map_user_stack(sb_address_space_t *space,
                          uint64_t bottom,
                          uint64_t top) {
    if (space == 0 || top <= bottom || ((top - bottom) % SB_PAGE_SIZE) != 0u) return -1;

    for (uint64_t va = bottom; va < top; va += SB_PAGE_SIZE) {
        void *page = pmm_alloc_page();
        if (page == 0) return -1;
        for (uint32_t i = 0u; i < SB_PAGE_SIZE; ++i) {
            ((uint8_t *)page)[i] = 0u;
        }
        if (address_space_map_user(space, va,
                                   (uint64_t)(uintptr_t)page,
                                   SB_VMM_WRITABLE | SB_VMM_NX) != 0) {
            pmm_free_page(page);
            return -1;
        }
    }
    return 0;
}

static void process_prepare_elf_rollback(sb_process_t *process) {
    if (process == 0) return;
    if (process->address_space.pml4_physical != 0u) {
        address_space_destroy(&process->address_space);
    }
    process->entry_point = 0u;
    process->user_stack_top = 0u;
    process->state = SB_PROCESS_CREATED;
}

int process_prepare_elf(sb_process_t *process,
                        const void *image,
                        uint64_t image_size,
                        sb_process_image_t *image_info) {
    if (process == 0 || image == 0 || image_size == 0u || image_info == 0 ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) return -1;

    *image_info = (sb_process_image_t){0};

    if (process->address_space.pml4_physical == 0u &&
        address_space_create(&process->address_space) != 0) {
        return -1;
    }

    uint64_t entry = 0u;
    if (elf64_load_image(&process->address_space, image, image_size, &entry) != 0) {
        process_prepare_elf_rollback(process);
        return -1;
    }

    const uint64_t stack_top = SB_USER_STACK_TOP;
    const uint64_t stack_bottom = stack_top - SB_USER_STACK_PAGES * SB_PAGE_SIZE;
    if (map_user_stack(&process->address_space, stack_bottom, stack_top) != 0) {
        process_prepare_elf_rollback(process);
        return -1;
    }

    process->state = SB_PROCESS_CREATED;
    process->entry_point = entry;
    process->user_stack_top = stack_top;

    image_info->entry_point = entry;
    image_info->user_stack_top = stack_top;
    image_info->user_stack_bottom = stack_bottom;
    return 0;
}

int process_prepare_boot_module(sb_process_t *process,
                                uint64_t multiboot_info,
                                const char *module_name,
                                sb_process_image_t *image_info) {
    sb_multiboot_module_t module;
    if (multiboot_find_module(multiboot_info, module_name, &module) != 0) return -1;
    if (module.end <= module.start) return -1;
    return process_prepare_elf(process,
                                (const void *)(uintptr_t)module.start,
                                module.end - module.start,
                                image_info);
}

int process_prepare_elf_thread(sb_process_t *process,
                               uint64_t tid,
                               uint32_t priority,
                               sb_user_context_t *context,
                               const sb_process_image_t *image_info,
                               sb_thread_t **thread_out) {
    if (process == 0 || context == 0 || image_info == 0 || thread_out == 0 ||
        image_info->entry_point == 0u || image_info->user_stack_top == 0u ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) return -1;

    *thread_out = 0;
    if (sb_user_context_init(context,
                             image_info->entry_point,
                             image_info->user_stack_top) != 0) return -1;

    sb_thread_t *thread = process_create_thread(process, tid, priority);
    if (thread == 0) return -1;

    thread->user_context = context;
    *thread_out = thread;
    return 0;
}
