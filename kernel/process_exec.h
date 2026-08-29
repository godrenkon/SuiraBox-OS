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

int process_prepare_elf_thread(sb_process_t *process,
                               uint64_t tid,
                               uint32_t priority,
                               sb_user_context_t *context,
                               const sb_process_image_t *image_info,
                               sb_thread_t **thread_out);

#endif
