#include "elf_loader.h"
#include "elf.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define PF_X 1u
#define PF_W 2u
#define PF_R 4u
#define SB_USER_LIMIT 0x0000800000000000ull
#define SB_PAGE_MASK (~(uint64_t)(SB_PAGE_SIZE - 1u))

static uint64_t align_down(uint64_t value) {
    return value & SB_PAGE_MASK;
}

static uint64_t align_up_checked(uint64_t value, uint64_t *out) {
    if (value > UINT64_MAX - (SB_PAGE_SIZE - 1u)) return 1u;
    *out = (value + (SB_PAGE_SIZE - 1u)) & SB_PAGE_MASK;
    return 0u;
}

static uint64_t range_end(uint64_t start, uint64_t size, uint64_t *end) {
    if (size > UINT64_MAX - start) return 1u;
    *end = start + size;
    return 0u;
}

int elf64_load_image(sb_address_space_t *space,
                     const void *image,
                     uint64_t image_size,
                     uint64_t *entry_point) {
    const sb_elf64_header_t *header;

    if (space == 0 || elf64_validate(image, image_size, &header) != 0) return -1;

    uint64_t load_bias = 0u;
    if (header->type == SB_ELF_TYPE_DYN) {
        load_bias = SB_USER_BASE - align_down(header->entry);
    }

    int entry_mapped = 0;

    for (uint16_t i = 0; i < header->phnum; ++i) {
        const uint64_t offset = header->phoff + (uint64_t)i * header->phentsize;
        const sb_elf64_program_header_t *ph =
            (const sb_elf64_program_header_t *)((const uint8_t *)image + offset);

        if (ph->type != SB_ELF_PT_LOAD || ph->memory_size == 0u) continue;

        uint64_t virtual_start;
        uint64_t virtual_end;
        if (range_end(load_bias, ph->virtual_address, &virtual_start) ||
            range_end(virtual_start, ph->memory_size, &virtual_end) ||
            virtual_start < SB_USER_BASE || virtual_end > SB_USER_LIMIT ||
            virtual_end <= virtual_start) {
            return -1;
        }

        uint64_t map_start = align_down(virtual_start);
        uint64_t map_end;
        if (align_up_checked(virtual_end, &map_end)) return -1;

        uint64_t flags = SB_VMM_USER;
        if ((ph->flags & PF_W) != 0u) flags |= SB_VMM_WRITABLE;
        if ((ph->flags & PF_X) == 0u) flags |= SB_VMM_NX;

        for (uint64_t va = map_start; va < map_end; va += SB_PAGE_SIZE) {
            void *page = pmm_alloc_page();
            if (page == 0) return -1;

            uint8_t *dst = (uint8_t *)page;
            for (uint32_t j = 0; j < SB_PAGE_SIZE; ++j) dst[j] = 0;

            uint64_t page_file_start = 0;
            uint64_t page_file_end = 0;
            uint64_t segment_file_start = 0;
            uint64_t segment_file_end = 0;
            if (range_end(ph->offset, ph->file_size, &segment_file_end)) return -1;
            segment_file_start = ph->offset;

            if (va < virtual_end && va + SB_PAGE_SIZE > virtual_start) {
                uint64_t copy_start = va > virtual_start ? va : virtual_start;
                uint64_t copy_end = (va + SB_PAGE_SIZE) < (virtual_start + ph->file_size)
                    ? (va + SB_PAGE_SIZE) : (virtual_start + ph->file_size);
                if (copy_end > copy_start &&
                    copy_start >= virtual_start &&
                    copy_end <= virtual_start + ph->file_size) {
                    page_file_start = segment_file_start + (copy_start - virtual_start);
                    page_file_end = segment_file_start + (copy_end - virtual_start);
                    if (page_file_start < segment_file_start || page_file_end > segment_file_end ||
                        page_file_end > image_size) return -1;
                    uint64_t dst_offset = copy_start - va;
                    const uint8_t *src = (const uint8_t *)image + page_file_start;
                    for (uint64_t j = 0; j < page_file_end - page_file_start; ++j) {
                        dst[dst_offset + j] = src[j];
                    }
                }
            }

            if (address_space_map_user(space, va,
                                       (uint64_t)(uintptr_t)page,
                                       flags) != 0) {
                pmm_free_page(page);
                return -1;
            }

            uint64_t page_end = va + SB_PAGE_SIZE;
            uint64_t entry_virtual = load_bias + header->entry;
            if (entry_virtual >= va && entry_virtual < page_end) entry_mapped = 1;
        }
    }

    if (!entry_mapped) return -1;
    if (entry_point != 0) *entry_point = load_bias + header->entry;
    return 0;
}
