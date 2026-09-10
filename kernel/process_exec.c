#include "process_exec.h"
#include "elf_loader.h"
#include "scheduler.h"
#include "mm/address_space.h"
#include "mm/multiboot_modules.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define SB_USER_STACK_PAGES 4u

static uint64_t registered_multiboot_info;

static int map_user_stack(sb_address_space_t *space,
                          uint64_t bottom,
                          uint64_t top) {
    if (space == 0 || top <= bottom || ((top - bottom) % SB_PAGE_SIZE) != 0u) return -1;

    for (uint64_t va = bottom; va < top; va += SB_PAGE_SIZE) {
        void *page = pmm_alloc_page();
        if (page == 0) return -1;
        for (uint32_t i = 0; i < SB_PAGE_SIZE; ++i) {
            ((uint8_t *)page)[i] = 0;
        }
        if (address_space_map_user(space, va,
                                   (uint64_t)(uintptr_t)page,
                                   SB_VMM_WRITABLE | SB_VMM_NX | SB_VMM_OWNED) != 0) {
            pmm_free_page(page);
            return -1;
        }
    }
    return 0;
}

int process_prepare_elf(sb_process_t *process,
                        const void *image,
                        uint64_t image_size,
                        sb_process_image_t *image_info) {
    if (process == 0 || image == 0 || image_size == 0u || image_info == 0) return -1;

    if (process->address_space.pml4_physical == 0u &&
        address_space_create(&process->address_space) != 0) {
        return -1;
    }

    uint64_t entry = 0;
    if (elf64_load_image(&process->address_space, image, image_size, &entry) != 0) {
        return -1;
    }

    const uint64_t stack_top = SB_USER_STACK_TOP;
    const uint64_t stack_bottom = stack_top - SB_USER_STACK_PAGES * SB_PAGE_SIZE;
    if (map_user_stack(&process->address_space, stack_bottom, stack_top) != 0) {
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
    if (multiboot_info == 0u ||
        multiboot_find_module(multiboot_info, module_name, &module) != 0) return -1;
    if (module.end <= module.start) return -1;

    const int result = process_prepare_elf(process,
                                           (const void *)(uintptr_t)module.start,
                                           module.end - module.start,
                                           image_info);
    if (result == 0 && registered_multiboot_info == 0u) {
        registered_multiboot_info = multiboot_info;
    }
    return result;
}

sb_process_t *process_spawn_elf_image(const void *image,
                                      uint64_t image_size,
                                      uint64_t pid,
                                      uint64_t parent_pid,
                                      uint64_t tid,
                                      uint32_t priority,
                                      sb_process_image_t *image_info) {
    if (image == 0 || image_size == 0u || pid == 0u || tid == 0u) return 0;
    if (parent_pid != 0u && process_get(parent_pid) == 0) return 0;

    sb_process_image_t local_image;
    sb_process_image_t *prepared = image_info != 0 ? image_info : &local_image;
    sb_process_t *process = process_create(pid);
    if (process == 0) return 0;
    process->parent_pid = parent_pid;

    if (process_prepare_elf(process, image, image_size, prepared) != 0) {
        process_destroy(process);
        return 0;
    }

    sb_thread_t *thread = process_create_thread(process, tid, priority);
    if (thread == 0) {
        process_destroy(process);
        return 0;
    }

    const int add_result = scheduler_add_user_task(thread->tid,
                                                   process->pid,
                                                   thread->priority,
                                                   process->address_space.pml4_physical,
                                                   prepared->entry_point,
                                                   prepared->user_stack_top);
    if (add_result != 0) {
        process_destroy(process);
        return 0;
    }

    process->state = SB_PROCESS_RUNNING;
    thread->state = SB_PROCESS_RUNNING;
    return process;
}

sb_process_t *process_spawn_boot_module(uint64_t multiboot_info,
                                        const char *module_name,
                                        uint64_t pid,
                                        uint64_t parent_pid,
                                        uint64_t tid,
                                        uint32_t priority,
                                        sb_process_image_t *image_info) {
    if (multiboot_info == 0u || module_name == 0) return 0;

    sb_multiboot_module_t module;
    if (multiboot_find_module(multiboot_info, module_name, &module) != 0 ||
        module.end <= module.start) {
        return 0;
    }

    sb_process_t *process = process_spawn_elf_image(
        (const void *)(uintptr_t)module.start,
        module.end - module.start,
        pid,
        parent_pid,
        tid,
        priority,
        image_info);
    if (process != 0 && registered_multiboot_info == 0u) {
        registered_multiboot_info = multiboot_info;
    }
    return process;
}

int process_registered_boot_module_exists(const char *module_name) {
    const void *image = 0;
    uint64_t image_size = 0u;
    return process_registered_boot_module_view(module_name, &image, &image_size) == 0;
}

int process_registered_boot_module_view(const char *module_name,
                                        const void **image_out,
                                        uint64_t *image_size_out) {
    if (registered_multiboot_info == 0u || module_name == 0 ||
        image_out == 0 || image_size_out == 0) {
        return -1;
    }

    sb_multiboot_module_t module;
    if (multiboot_find_module(registered_multiboot_info,
                              module_name,
                              &module) != 0 ||
        module.end <= module.start) {
        return -1;
    }

    *image_out = (const void *)(uintptr_t)module.start;
    *image_size_out = module.end - module.start;
    return 0;
}

sb_process_t *process_spawn_registered_boot_module(const char *module_name,
                                                   uint64_t pid,
                                                   uint64_t parent_pid,
                                                   uint64_t tid,
                                                   uint32_t priority,
                                                   sb_process_image_t *image_info) {
    const void *image = 0;
    uint64_t image_size = 0u;
    if (process_registered_boot_module_view(module_name, &image, &image_size) != 0) {
        return 0;
    }

    return process_spawn_elf_image(image,
                                   image_size,
                                   pid,
                                   parent_pid,
                                   tid,
                                   priority,
                                   image_info);
}
