#ifndef SB_KERNEL_MULTIBOOT_MODULES_H
#define SB_KERNEL_MULTIBOOT_MODULES_H

#include <stdint.h>

typedef struct {
    uint64_t start;
    uint64_t end;
    const char *name;
    uint32_t name_length;
} sb_multiboot_module_t;

int multiboot_find_module(uint64_t multiboot_info,
                          const char *name,
                          sb_multiboot_module_t *module);

/* Returns the index-th valid module tag. name points at the Multiboot-owned
 * command line and name_length covers only its first token (the image name). */
int multiboot_module_at(uint64_t multiboot_info,
                        uint32_t index,
                        sb_multiboot_module_t *module);

#endif
