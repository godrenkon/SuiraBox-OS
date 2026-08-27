#ifndef SB_MULTIBOOT_MEMORY_H
#define SB_MULTIBOOT_MEMORY_H

#include <stdint.h>

#define MULTIBOOT2_TAG_TYPE_END        0u
#define MULTIBOOT2_TAG_TYPE_MODULE     3u
#define MULTIBOOT2_TAG_TYPE_MMAP       6u
#define MULTIBOOT2_MMAP_TYPE_AVAILABLE 1u

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot2_module_tag {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string[0];
};

struct multiboot2_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

#endif
