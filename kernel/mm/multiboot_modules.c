#include "multiboot_modules.h"

#define MULTIBOOT2_TAG_MODULE 3u
#define MULTIBOOT2_TAG_END 0u

struct __attribute__((packed)) multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct __attribute__((packed)) multiboot_module_tag {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[];
};

static int str_equal(const char *a, const char *b) {
    if (a == 0 || b == 0) return 0;
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static const char *module_name(const char *cmdline) {
    const char *start = cmdline;
    const char *p = cmdline;
    while (*p != '\0' && *p != ' ') ++p;
    return start;
}

int multiboot_find_module(uint64_t multiboot_info,
                          const char *name,
                          sb_multiboot_module_t *module) {
    if (multiboot_info == 0 || name == 0 || module == 0) return -1;

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info;
    if (total_size < 16u) return -1;

    uint32_t offset = 8u;
    while (offset + sizeof(struct multiboot_tag) <= total_size) {
        const struct multiboot_tag *tag =
            (const struct multiboot_tag *)(uintptr_t)(multiboot_info + offset);
        if (tag->type == MULTIBOOT2_TAG_END) break;
        if (tag->size < sizeof(struct multiboot_tag) || offset + tag->size > total_size) break;

        if (tag->type == MULTIBOOT2_TAG_MODULE &&
            tag->size >= sizeof(struct multiboot_module_tag)) {
            const struct multiboot_module_tag *mod =
                (const struct multiboot_module_tag *)tag;
            if (mod->mod_end > mod->mod_start && str_equal(module_name(mod->cmdline), name)) {
                module->start = mod->mod_start;
                module->end = mod->mod_end;
                module->name = mod->cmdline;
                return 0;
            }
        }

        offset += (tag->size + 7u) & ~7u;
    }
    return -1;
}
