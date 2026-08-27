#ifndef SB_KERNEL_MULTIBOOT_MODULES_H
#define SB_KERNEL_MULTIBOOT_MODULES_H

#include <stdint.h>

typedef struct {
    uint64_t start;
    uint64_t end;
    const char *name;
} sb_multiboot_module_t;

int multiboot_find_module(uint64_t multiboot_info,
                          const char *name,
                          sb_multiboot_module_t *module);

#endif
