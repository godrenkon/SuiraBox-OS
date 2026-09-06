#ifndef SB_KERNEL_PROCESS_EXEC_H
#define SB_KERNEL_PROCESS_EXEC_H

#include <stdint.h>
#include "process.h"

typedef struct {
    uint64_t entry_point;
    uint64_t user_stack_top;
    uint64_t user_stack_bottom;
} sb_process_image_t;

int process_prepare_elf(sb_process_t *process,
                        const void *image,
                        uint64_t image_size,
                        sb_process_image_t *image_info);

int process_prepare_boot_module(sb_process_t *process,
                                uint64_t multiboot_info,
                                const char *module_name,
                                sb_process_image_t *image_info);

/* Create, load, create the initial thread, and register it with the scheduler.
 * Failure is transactional: no process object or address-space pages remain. */
sb_process_t *process_spawn_boot_module(uint64_t multiboot_info,
                                        const char *module_name,
                                        uint64_t pid,
                                        uint64_t tid,
                                        uint32_t priority,
                                        sb_process_image_t *image_info);

/* Bootstrap bridge used until VFS-backed executable lookup exists. The first
 * successful boot-module load records the Multiboot information pointer, so a
 * later userspace SPAWN syscall can select another trusted boot module without
 * accepting an unchecked userspace string pointer. */
sb_process_t *process_spawn_registered_boot_module(const char *module_name,
                                                   uint64_t pid,
                                                   uint64_t tid,
                                                   uint32_t priority,
                                                   sb_process_image_t *image_info);

#endif
