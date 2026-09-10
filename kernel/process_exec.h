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

/* Generic memory-image spawn boundary. The caller owns image memory and only
 * needs to keep it alive until this function returns; ELF segments are copied
 * into the new process address space. Failure is transactional. */
sb_process_t *process_spawn_elf_image(const void *image,
                                      uint64_t image_size,
                                      uint64_t pid,
                                      uint64_t parent_pid,
                                      uint64_t tid,
                                      uint32_t priority,
                                      sb_process_image_t *image_info);

/* Create, load, create the initial thread, and register it with the scheduler.
 * Failure is transactional: no process object or address-space pages remain. */
sb_process_t *process_spawn_boot_module(uint64_t multiboot_info,
                                        const char *module_name,
                                        uint64_t pid,
                                        uint64_t parent_pid,
                                        uint64_t tid,
                                        uint32_t priority,
                                        sb_process_image_t *image_info);

/* Bootstrap registry used until a persistent filesystem-backed executable
 * namespace is mounted. These helpers return kernel-owned immutable module
 * bytes; callers must not free or modify the returned image. */
int process_registered_boot_module_exists(const char *module_name);
int process_registered_boot_module_view(const char *module_name,
                                        const void **image_out,
                                        uint64_t *image_size_out);

sb_process_t *process_spawn_registered_boot_module(const char *module_name,
                                                   uint64_t pid,
                                                   uint64_t parent_pid,
                                                   uint64_t tid,
                                                   uint32_t priority,
                                                   sb_process_image_t *image_info);

#endif
