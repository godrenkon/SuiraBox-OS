#include "multiboot_modules.h"
#include <stddef.h>

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

static int parse_module_tag(const struct multiboot_module_tag *mod,
                            sb_multiboot_module_t *module) {
    if (mod == 0 || module == 0 ||
        mod->size < sizeof(struct multiboot_module_tag) + 1u ||
        mod->mod_end <= mod->mod_start) {
        return -1;
    }

    const uint32_t cmdline_bytes =
        mod->size - (uint32_t)offsetof(struct multiboot_module_tag, cmdline);
    uint32_t token_length = 0u;
    uint32_t nul_index = cmdline_bytes;
    for (uint32_t i = 0u; i < cmdline_bytes; ++i) {
        const char c = mod->cmdline[i];
        if (c == '\0') {
            nul_index = i;
            break;
        }
        if (token_length == i && c != ' ' && c != '\t') {
            ++token_length;
        }
    }

    if (nul_index == cmdline_bytes || token_length == 0u) return -1;

    module->start = mod->mod_start;
    module->end = mod->mod_end;
    module->name = mod->cmdline;
    module->name_length = token_length;
    return 0;
}

static int module_name_equal(const sb_multiboot_module_t *module,
                             const char *name) {
    if (module == 0 || name == 0 || *name == '\0') return 0;

    uint32_t length = 0u;
    while (name[length] != '\0') {
        if (length >= module->name_length || module->name[length] != name[length]) return 0;
        ++length;
    }
    return length == module->name_length;
}

static int next_tag(uint32_t total_size,
                    uint32_t current_offset,
                    uint32_t tag_size,
                    uint32_t *next_offset) {
    if (next_offset == 0 || tag_size < sizeof(struct multiboot_tag)) return -1;
    const uint32_t aligned = (tag_size + 7u) & ~7u;
    if (aligned < tag_size || current_offset > total_size - aligned) return -1;
    *next_offset = current_offset + aligned;
    return 0;
}

int multiboot_module_at(uint64_t multiboot_info,
                        uint32_t index,
                        sb_multiboot_module_t *module) {
    if (multiboot_info == 0u || module == 0) return -1;

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info;
    if (total_size < 16u) return -1;

    uint32_t offset = 8u;
    uint32_t visible_index = 0u;
    while (offset <= total_size - sizeof(struct multiboot_tag)) {
        const struct multiboot_tag *tag =
            (const struct multiboot_tag *)(uintptr_t)(multiboot_info + offset);
        if (tag->type == MULTIBOOT2_TAG_END) return -1;
        if (tag->size < sizeof(struct multiboot_tag) || tag->size > total_size - offset) {
            return -1;
        }

        if (tag->type == MULTIBOOT2_TAG_MODULE &&
            tag->size >= sizeof(struct multiboot_module_tag) + 1u) {
            sb_multiboot_module_t candidate;
            if (parse_module_tag((const struct multiboot_module_tag *)tag,
                                 &candidate) == 0) {
                if (visible_index == index) {
                    *module = candidate;
                    return 0;
                }
                if (visible_index == UINT32_MAX) return -1;
                ++visible_index;
            }
        }

        uint32_t next = 0u;
        if (next_tag(total_size, offset, tag->size, &next) != 0 || next <= offset) {
            return -1;
        }
        offset = next;
    }
    return -1;
}

int multiboot_find_module(uint64_t multiboot_info,
                          const char *name,
                          sb_multiboot_module_t *module) {
    if (multiboot_info == 0u || name == 0 || module == 0) return -1;

    for (uint32_t index = 0u;; ++index) {
        sb_multiboot_module_t candidate;
        if (multiboot_module_at(multiboot_info, index, &candidate) != 0) return -1;
        if (module_name_equal(&candidate, name)) {
            *module = candidate;
            return 0;
        }
        if (index == UINT32_MAX) return -1;
    }
}
