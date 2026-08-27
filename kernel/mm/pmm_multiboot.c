#include "pmm.h"
#include "multiboot_memory.h"

extern char __kernel_start;
extern char __kernel_end;

#define SB_MB2_MAX_INFO_SIZE (64u * 1024u)
#define SB_MB2_MAX_TAGS 256u
#define SB_MB2_MAX_MMAP_ENTRIES 4096u

static uint64_t align_up(uint64_t value) {
    if (value > UINT64_MAX - (SB_PAGE_SIZE - 1u)) return UINT64_MAX;
    return (value + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);
}

static uint64_t align_down(uint64_t value) {
    return value & ~(uint64_t)(SB_PAGE_SIZE - 1u);
}

static void reserve_u32_range(uint32_t start, uint32_t end) {
    if (end <= start) return;
    pmm_reserve_range(align_down((uint64_t)start), align_up((uint64_t)end));
}

static void pmm_safe_fallback(uint64_t kernel_start, uint64_t kernel_end) {
    pmm_reset();
    /* Keep the low bootstrap region reserved; use only the remainder below 64 MiB. */
    pmm_add_usable_range(16u * 1024u * 1024u, 64u * 1024u * 1024u);
    pmm_reserve_range(align_down(kernel_start), align_up(kernel_end));
}

void pmm_init_from_multiboot(uint64_t multiboot_info_address) {
    const uint64_t kernel_start = (uint64_t)(uintptr_t)&__kernel_start;
    const uint64_t kernel_end = (uint64_t)(uintptr_t)&__kernel_end;
    const struct multiboot2_tag *tag;
    uint32_t offset = 8u;
    uint32_t tags_seen = 0u;
    int valid_map = 0;

    pmm_reset();

    if (multiboot_info_address == 0u || multiboot_info_address >= 0x40000000ull) {
        pmm_safe_fallback(kernel_start, kernel_end);
        return;
    }

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_address;
    if (total_size < 16u || total_size > SB_MB2_MAX_INFO_SIZE) {
        pmm_safe_fallback(kernel_start, kernel_end);
        return;
    }

    while (offset <= total_size - 8u && tags_seen++ < SB_MB2_MAX_TAGS) {
        tag = (const struct multiboot2_tag *)(uintptr_t)(multiboot_info_address + offset);
        if (tag->size < 8u || tag->size > total_size - offset) break;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) {
            valid_map = 1;
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE &&
            tag->size >= sizeof(struct multiboot2_module_tag)) {
            const struct multiboot2_module_tag *module =
                (const struct multiboot2_module_tag *)tag;
            reserve_u32_range(module->mod_start, module->mod_end);
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP &&
                   tag->size >= sizeof(struct multiboot2_mmap_tag)) {
            const struct multiboot2_mmap_tag *mmap =
                (const struct multiboot2_mmap_tag *)tag;
            if (mmap->entry_size >= sizeof(struct multiboot2_mmap_entry) &&
                mmap->entry_size <= tag->size - sizeof(struct multiboot2_mmap_tag)) {
                uint32_t entry_offset = sizeof(struct multiboot2_mmap_tag);
                uint32_t entries_seen = 0u;
                while (entry_offset <= tag->size - mmap->entry_size &&
                       entries_seen++ < SB_MB2_MAX_MMAP_ENTRIES) {
                    const struct multiboot2_mmap_entry *entry =
                        (const struct multiboot2_mmap_entry *)((const uint8_t *)mmap + entry_offset);
                    if (entry->type == MULTIBOOT2_MMAP_TYPE_AVAILABLE && entry->len != 0u &&
                        entry->len <= UINT64_MAX - entry->addr) {
                        pmm_add_usable_range(entry->addr, entry->addr + entry->len);
                    }
                    entry_offset += mmap->entry_size;
                }
            }
        }

        const uint32_t next_offset = (tag->size + 7u) & ~7u;
        if (next_offset < tag->size || offset > total_size - next_offset) break;
        offset += next_offset;
    }

    if (!valid_map || pmm_free_pages() == 0u) {
        pmm_safe_fallback(kernel_start, kernel_end);
        return;
    }

    /* Never hand the kernel image, stack, page tables, or bitmap to callers. */
    pmm_reserve_range(align_down(kernel_start), align_up(kernel_end));
}
