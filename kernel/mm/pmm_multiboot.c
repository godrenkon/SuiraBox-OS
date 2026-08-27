#include "pmm.h"
#include "multiboot_memory.h"

extern char __kernel_start;
extern char __kernel_end;

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

void pmm_init_from_multiboot(uint64_t multiboot_info_address) {
    const uint64_t kernel_start = (uint64_t)(uintptr_t)&__kernel_start;
    const uint64_t kernel_end = (uint64_t)(uintptr_t)&__kernel_end;
    const struct multiboot2_tag *tag;
    uint32_t offset = 8u;

    pmm_reset();

    if (multiboot_info_address == 0u) {
        pmm_reserve_range(0u, UINT64_MAX);
        return;
    }

    const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_address;
    if (total_size < 16u) {
        pmm_reserve_range(0u, UINT64_MAX);
        return;
    }

    while (offset + 8u <= total_size) {
        tag = (const struct multiboot2_tag *)(uintptr_t)(multiboot_info_address + offset);
        if (tag->size < 8u || offset + tag->size > total_size) break;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE &&
            tag->size >= sizeof(struct multiboot2_module_tag)) {
            const struct multiboot2_module_tag *module =
                (const struct multiboot2_module_tag *)tag;
            reserve_u32_range(module->mod_start, module->mod_end);
        }

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP &&
            tag->size >= sizeof(struct multiboot2_mmap_tag)) {
            const struct multiboot2_mmap_tag *mmap =
                (const struct multiboot2_mmap_tag *)tag;
            if (mmap->entry_size >= sizeof(struct multiboot2_mmap_entry)) {
                uint32_t entry_offset = sizeof(struct multiboot2_mmap_tag);
                while (entry_offset + mmap->entry_size <= mmap->size &&
                       entry_offset + mmap->entry_size <= tag->size) {
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

        offset += (tag->size + 7u) & ~7u;
    }

    /* Never hand the kernel image, stack, page tables, or bitmap to callers. */
    pmm_reserve_range(align_down(kernel_start), align_up(kernel_end));
}
