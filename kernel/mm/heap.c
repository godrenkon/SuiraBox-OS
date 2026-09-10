#include "heap.h"
#include "pmm.h"
#include <stdint.h>

#define KHEAP_MAX_ALLOCATIONS 64u

typedef struct {
    void *base;
    uint64_t pages;
    uint64_t requested;
    uint8_t in_use;
} kheap_allocation_t;

static kheap_allocation_t allocations[KHEAP_MAX_ALLOCATIONS];
static uint64_t used_bytes;
static uint64_t backed_bytes;
static int initialized;

static uint64_t round_up_pages(uint64_t size) {
    if (size == 0u || size > UINT64_MAX - (SB_PAGE_SIZE - 1u)) return 0u;
    return (size + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
}

void kheap_init(void) {
    for (uint32_t i = 0u; i < KHEAP_MAX_ALLOCATIONS; ++i) {
        allocations[i] = (kheap_allocation_t){0};
    }
    used_bytes = 0u;
    backed_bytes = 0u;
    initialized = 1;
}

void *kheap_alloc(uint64_t size) {
    if (!initialized || size == 0u || used_bytes > UINT64_MAX - size) return 0;

    const uint64_t pages = round_up_pages(size);
    if (pages == 0u || pages > UINT64_MAX / SB_PAGE_SIZE) return 0;

    uint32_t slot = KHEAP_MAX_ALLOCATIONS;
    for (uint32_t i = 0u; i < KHEAP_MAX_ALLOCATIONS; ++i) {
        if (allocations[i].in_use == 0u) {
            slot = i;
            break;
        }
    }
    if (slot == KHEAP_MAX_ALLOCATIONS) return 0;

    void *base = pmm_alloc_contiguous(pages);
    if (base == 0) return 0;

    allocations[slot].base = base;
    allocations[slot].pages = pages;
    allocations[slot].requested = size;
    allocations[slot].in_use = 1u;
    used_bytes += size;
    backed_bytes += pages * SB_PAGE_SIZE;
    return base;
}

void kheap_free(void *ptr) {
    if (!initialized || ptr == 0) return;

    for (uint32_t i = 0u; i < KHEAP_MAX_ALLOCATIONS; ++i) {
        kheap_allocation_t *allocation = &allocations[i];
        if (allocation->in_use == 0u || allocation->base != ptr) continue;

        pmm_free_contiguous(allocation->base, allocation->pages);
        if (used_bytes >= allocation->requested) used_bytes -= allocation->requested;
        else used_bytes = 0u;

        const uint64_t allocation_bytes = allocation->pages * SB_PAGE_SIZE;
        if (backed_bytes >= allocation_bytes) backed_bytes -= allocation_bytes;
        else backed_bytes = 0u;

        *allocation = (kheap_allocation_t){0};
        return;
    }
}

uint64_t kheap_used(void) { return used_bytes; }
uint64_t kheap_capacity(void) { return backed_bytes; }
