#ifndef SB_KERNEL_PROCESS_EXEC_H
#define SB_KERNEL_PROCESS_EXEC_H

#include <stdint.h>
#include "process.h"
#include "vfs_object.h"

#define SB_PROCESS_EXEC_MAX_IMAGE_SIZE (16u * 1024u * 1024u)

typedef struct {
    uint64_t entry_point;
    uint64_t user_stack_top;
    uint64_t user_stack_bottom;
} sb_process_image_t;

typedef struct {
    const char *name;
    uint32_t name_length;
    const void *image;
    uint64_t image_size;
} sb_registered_boot_module_t;

int process_prepare_elf(sb_process_t *process,
                        const void *image,
                        uint64_t image_size,
                        sb_process_image_t *image_info);

int process_prepare_boot_module(sb_process_t *process,
                                uint64_t multiboot_info,
                                const char *module_name,
                                sb_process_image_t *image_info);

sb_process_t *process_spawn_elf_image(const void *image,
                                      uint64_t image_size,
                                      uint64_t pid,
                                      uint64_t parent_pid,
                                      uint64_t tid,
                                      uint32_t priority,
                                      sb_process_image_t *image_info);

sb_process_t *process_spawn_vfs_file(sb_vfs_file_t *file,
                                     uint64_t pid,
                                     uint64_t parent_pid,
                                     uint64_t tid,
                                     uint32_t priority,
                                     sb_process_image_t *image_info);

sb_process_t *process_spawn_boot_module(uint64_t multiboot_info,
                                        const char *module_name,
                                        uint64_t pid,
                                        uint64_t parent_pid,
                                        uint64_t tid,
                                        uint32_t priority,
                                        sb_process_image_t *image_info);

/* Add a second or later ring3 thread to an existing process. The entry address
 * must already belong to the process address space. Each thread receives a
 * private four-page user stack separated from the previous stack by one
 * unmapped guard page. On failure, newly mapped stack pages are rolled back. */
int process_spawn_user_thread(sb_process_t *process,
                              uint64_t user_entry,
                              uint32_t priority,
                              uint64_t *tid_out);

int process_registered_boot_module_exists(const char *module_name);
int process_registered_boot_module_view(const char *module_name,
                                        const void **image_out,
                                        uint64_t *image_size_out);
int process_registered_boot_module_at(uint32_t index,
                                      sb_registered_boot_module_t *module_out);

sb_process_t *process_spawn_registered_boot_module(const char *module_name,
                                                   uint64_t pid,
                                                   uint64_t parent_pid,
                                                   uint64_t tid,
                                                   uint32_t priority,
                                                   sb_process_image_t *image_info);

#endif