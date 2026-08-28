#include "pmm.h"
#include "multiboot_memory.h"

extern char __kernel_start;
extern char __kernel_end;

#define SB_MB2_MAX_INFO_SIZE (64u * 1024u)
#define SB_MB2_MAX_TAGS 128u
#define SB_MB2_MAX_MMAP_ENTRIES 128u
#define SB_MB2_MAX_MODULES 32u
#define SB_PMM_BOOTSTRAP_LIMIT (256ull * 1024ull * 1024ull)
#define SB_NULL_PAGE_END SB_PAGE_SIZE

static uint64_t align_up(uint64_t value) {
    if (value > UINT64_MAX - (SB_PAGE_SIZE - 1u)) return UINT64_MAX;
    return (value + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);
}

static uint64_t align_down(uint64_t value) {
    return value & ~(uint64_t)(SB_PAGE_SIZE - 1u);
}

static void reserve_range(uint64_t start, uint64_t end) {
    if (end <= start) return;
    if (start >= SB_PMM_BOOTSTRAP_LIMIT) return;
    if (end > SB_PMM_BOOTSTRAP_LIMIT) end = SB_PMM_BOOTSTRAP_LIMIT;
    start = align_down(start);
    end = align_up(end);
    if (end > start) pmm_reserve_range(start, end);
}

static void fallback_to_bootstrap(void) {
    pmm_init(0x01000000ull, 0x04000000ull);
    pmm_reserve_range((uint64_t)(uintptr_t)&__kernel_start,
                      (uint64_t)(uintptr_t)&__kernel_end);
    pmm_reserve_range(0u, SB_NULL_PAGE_END);
}

void pmm_init_from_multiboot(uint64_t multiboot_info_address) {
    const uint64_t kernel_start = (uint64_t)(uintptr_t)&__kernel_start;
    const uint64_t kernel_end = (uint64_t)(uintptr_t)&__kernel_end;
    const struct multiboot2_tag *tag;
    uint32_t offset = 8u;
    uint32_t tags_seen = 0u;
    uint32_t mmap_entries_seen = 0u;
    uint32_t modules_seen = 0u;
    int valid_end = 0;
    int seen_mmap = 0;

    pmm_reset();
    pmm_reserve_range(0u, SB_NULL_PAGE_END);

    if (multiboot_info_address == 0u || multiboot_info_address >= 0x40000000ull) {
        fallback_to_bootstrap();
        return;
    }

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_address;
    if (total_size < 16u || total_size > SB_MB2_MAX_INFO_SIZE ||
        (total_size & 7u) != 0u || total_size > UINT64_MAX - multiboot_info_address) {
        fallback_to_bootstrap();
        return;
    }

    while (offset <= total_size - 8u && tags_seen++ < SB_MB2_MAX_TAGS) {
        tag = (const struct multiboot2_tag *)(uintptr_t)(multiboot_info_address + offset);
        if (tag->size < 8u || tag->size > total_size - offset) {
            fallback_to_bootstrap();
            return;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            valid_end = tag->size == 8u;
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
            if (tag->size < sizeof(struct multiboot2_module_tag) ||
                modules_seen >= SB_MB2_MAX_MODULES) {
                fallback_to_bootstrap();
                return;
            }
            const struct multiboot2_module_tag *module =
                (const struct multiboot2_module_tag *)tag;
            reserve_range(module->mod_start, module->mod_end);
            ++modules_seen;
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            if (tag->size < sizeof(struct multiboot2_mmap_tag)) {
                fallback_to_bootstrap();
                return;
            }
            const struct multiboot2_mmap_tag *mmap =
                (const struct multiboot2_mmap_tag *)tag;
            if (mmap->entry_size < sizeof(struct multiboot2_mmap_entry) ||
                mmap->entry_size > tag->size - sizeof(struct multiboot2_mmap_tag)) {
                fallback_to_bootstrap();
                return;
            }

            seen_mmap = 1;
            uint32_t entry_offset = sizeof(struct multiboot2_mmap_tag);
            while (entry_offset <= tag->size - mmap->entry_size) {
                if (mmap_entries_seen++ >= SB_MB2_MAX_MMAP_ENTRIES) {
                    fallback_to_bootstrap();
                    return;
                }
                const struct multiboot2_mmap_entry *entry =
                    (const struct multiboot2_mmap_entry *)((const uint8_t *)mmap + entry_offset);
                if (entry->len != 0u && entry->len <= UINT64_MAX - entry->addr) {
                    const uint64_t start = entry->addr;
                    const uint64_t end = entry->addr + entry->len;
                    if (entry->type == MULTIBOOT2_MMAP_TYPE_AVAILABLE) {
                        if (start < SB_PMM_BOOTSTRAP_LIMIT) {
                            uint64_t usable_end = end;
                            if (usable_end > SB_PMM_BOOTSTRAP_LIMIT)
                                usable_end = SB_PMM_BOOTSTRAP_LIMIT;
                            usable_end = align_down(usable_end);
                            if (usable_end > start)
                                pmm_add_usable_range(align_up(start), usable_end);
                        }
                    } else {
                        reserve_range(start, end);
                    }
                }
                entry_offset += mmap->entry_size;
            }
        }

        const uint32_t next_offset = (tag->size + 7u) & ~7u;
        if (next_offset < tag->size || next_offset > total_size - offset) {
            fallback_to_bootstrap();
            return;
        }
        offset += next_offset;
    }

    if (!valid_end || !seen_mmap) {
        fallback_to_bootstrap();
        return;
    }

    reserve_range(kernel_start, kernel_end);
    if (multiboot_info_address <= UINT64_MAX - total_size)
        reserve_range(multiboot_info_address, multiboot_info_address + total_size);
    else
        fallback_to_bootstrap();
    pmm_reserve_range(0u, SB_NULL_PAGE_END);
}
