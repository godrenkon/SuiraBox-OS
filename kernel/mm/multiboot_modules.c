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

static int module_name_equal(const char *cmdline, const char *name) {
    if (cmdline == 0 || name == 0 || *name == '\0') return 0;

    while (*name != '\0') {
        if (*cmdline != *name) return 0;
        ++cmdline;
        ++name;
    }

    /* A Multiboot module command line may carry options after its image name.
     * Only the first token identifies the module. */
    return *cmdline == '\0' || *cmdline == ' ' || *cmdline == '\t';
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
            if (mod->mod_end > mod->mod_start &&
                module_name_equal(mod->cmdline, name)) {
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
