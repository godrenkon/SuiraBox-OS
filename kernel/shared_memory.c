#include "shared_memory.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include <stdint.h>

int sb_shared_memory_create(uint64_t size, sb_shared_memory_t **memory_out) {
    if (memory_out != 0) *memory_out = 0;
    if (memory_out == 0 || size == 0u ||
        size > (uint64_t)SB_SHARED_MEMORY_MAX_PAGES * SB_PAGE_SIZE) {
        return -1;
    }

    const uint64_t pages = (size + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
    sb_shared_memory_t *memory =
        (sb_shared_memory_t *)kheap_alloc(sizeof(*memory));
    if (memory == 0) return -2;

    void *backing = pmm_alloc_contiguous(pages);
    if (backing == 0) {
        kheap_free(memory);
        return -2;
    }

    uint8_t *bytes = (uint8_t *)backing;
    const uint64_t backing_bytes = pages * SB_PAGE_SIZE;
    for (uint64_t i = 0u; i < backing_bytes; ++i) bytes[i] = 0u;

    memory->physical_base = (uint64_t)(uintptr_t)backing;
    memory->size = size;
    memory->page_count = (uint32_t)pages;
    memory->ref_count = 1u;
    *memory_out = memory;
    return 0;
}

int sb_shared_memory_retain(sb_shared_memory_t *memory) {
    if (memory == 0 || memory->physical_base == 0u ||
        memory->page_count == 0u || memory->ref_count == 0u ||
        memory->ref_count == UINT32_MAX) {
        return -1;
    }
    ++memory->ref_count;
    return 0;
}

void sb_shared_memory_release(sb_shared_memory_t *memory) {
    if (memory == 0 || memory->ref_count == 0u) return;
    --memory->ref_count;
    if (memory->ref_count != 0u) return;

    if (memory->physical_base != 0u && memory->page_count != 0u) {
        pmm_free_contiguous((void *)(uintptr_t)memory->physical_base,
                            memory->page_count);
    }
    memory->physical_base = 0u;
    memory->page_count = 0u;
    memory->size = 0u;
    kheap_free(memory);
}
