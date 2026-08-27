#include "heap.h"
#include "pmm.h"

#define KHEAP_MAX_ALLOCS 128u

typedef struct {
    void *base;
    uint64_t pages;
    uint64_t requested;
} kheap_record_t;

static kheap_record_t records[KHEAP_MAX_ALLOCS];
static uint64_t used_bytes;
static int initialized;

static uint64_t round_up_pages(uint64_t size) {
    return (size + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
}

void kheap_init(void) {
    for (uint32_t i = 0; i < KHEAP_MAX_ALLOCS; ++i) {
        records[i].base = 0;
        records[i].pages = 0;
        records[i].requested = 0;
    }
    used_bytes = 0;
    initialized = 1;
}

void *kheap_alloc(uint64_t size) {
    if (!initialized || size == 0u) {
        return 0;
    }

    uint64_t pages = round_up_pages(size);
    if (pages == 0u) {
        return 0;
    }

    uint32_t slot = 0;
    while (slot < KHEAP_MAX_ALLOCS && records[slot].base != 0) {
        ++slot;
    }
    if (slot == KHEAP_MAX_ALLOCS) {
        return 0;
    }

    /* Contiguous physical pages are required because the bootstrap VMM identity-maps them. */
    void *first = 0;
    for (uint64_t start = 0; start + pages <= pmm_total_pages(); ++start) {
        /* pmm_alloc_page() is intentionally used only for single pages in the bootstrap.
           Heap growth therefore remains conservative until a contiguous allocator exists. */
        (void)start;
        break;
    }

    /* Allocate one page at a time only for single-page requests in this bootstrap heap. */
    if (pages != 1u) {
        return 0;
    }

    first = pmm_alloc_page();
    if (first == 0) {
        return 0;
    }

    records[slot].base = first;
    records[slot].pages = 1;
    records[slot].requested = size;
    used_bytes += size;
    return first;
}

void kheap_free(void *ptr) {
    if (!initialized || ptr == 0) {
        return;
    }

    for (uint32_t i = 0; i < KHEAP_MAX_ALLOCS; ++i) {
        if (records[i].base != ptr) {
            continue;
        }

        pmm_free_page(records[i].base);
        if (used_bytes >= records[i].requested) {
            used_bytes -= records[i].requested;
        } else {
            used_bytes = 0;
        }
        records[i].base = 0;
        records[i].pages = 0;
        records[i].requested = 0;
        return;
    }
}

uint64_t kheap_used(void) {
    return used_bytes;
}

uint64_t kheap_capacity(void) {
    return (uint64_t)KHEAP_MAX_ALLOCS * SB_PAGE_SIZE;
}
