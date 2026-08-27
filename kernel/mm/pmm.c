#include "pmm.h"

/* Bootstrap PMM tracks the first 64 MiB of physical memory in 4 KiB pages. */
#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_BITMAP_WORDS ((PMM_MAX_PAGES + 63u) / 64u)

static uint64_t bitmap[PMM_BITMAP_WORDS];
static uint64_t page_count;
static uint64_t free_count;

static uint64_t address_to_index(uint64_t address) {
    return address / SB_PAGE_SIZE;
}

static int page_index_valid(uint64_t index) {
    return index < PMM_MAX_PAGES;
}

static void mark_used(uint64_t index) {
    bitmap[index / 64u] |= (1ull << (index % 64u));
}

static void mark_free(uint64_t index) {
    bitmap[index / 64u] &= ~(1ull << (index % 64u));
}

static int is_free(uint64_t index) {
    return (bitmap[index / 64u] & (1ull << (index % 64u))) == 0u;
}

void pmm_reset(void) {
    for (uint32_t i = 0; i < PMM_BITMAP_WORDS; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    page_count = PMM_MAX_PAGES;
    free_count = 0;
}

void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end) {
    const uint64_t max_address = (uint64_t)PMM_MAX_PAGES * SB_PAGE_SIZE;

    if (usable_start >= max_address || usable_end <= usable_start) {
        return;
    }
    if (usable_end > max_address) {
        usable_end = max_address;
    }

    usable_start = (usable_start + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);
    usable_end &= ~(uint64_t)(SB_PAGE_SIZE - 1u);

    for (uint64_t address = usable_start; address < usable_end; address += SB_PAGE_SIZE) {
        const uint64_t index = address_to_index(address);
        if (!page_index_valid(index) || is_free(index)) {
            continue;
        }
        mark_free(index);
        ++free_count;
    }
}

void pmm_reserve_range(uint64_t start, uint64_t end) {
    const uint64_t max_address = (uint64_t)PMM_MAX_PAGES * SB_PAGE_SIZE;

    if (start >= max_address || end <= start) {
        return;
    }
    if (end > max_address) {
        end = max_address;
    }

    start &= ~(uint64_t)(SB_PAGE_SIZE - 1u);
    end = (end + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);

    for (uint64_t address = start; address < end; address += SB_PAGE_SIZE) {
        const uint64_t index = address_to_index(address);
        if (!page_index_valid(index) || !is_free(index)) {
            continue;
        }
        mark_used(index);
        --free_count;
    }
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    pmm_reset();
    pmm_add_usable_range(usable_start, usable_end);
}

void *pmm_alloc_page(void) {
    for (uint64_t i = 0; i < page_count; ++i) {
        if (is_free(i)) {
            mark_used(i);
            --free_count;
            return (void *)(uintptr_t)(i * SB_PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free_page(void *page) {
    const uint64_t address = (uint64_t)(uintptr_t)page;
    if ((address & (SB_PAGE_SIZE - 1u)) != 0u) {
        return;
    }

    const uint64_t index = address_to_index(address);
    if (!page_index_valid(index) || is_free(index)) {
        return;
    }

    mark_free(index);
    ++free_count;
}

uint64_t pmm_total_pages(void) {
    return page_count;
}

uint64_t pmm_free_pages(void) {
    return free_count;
}
