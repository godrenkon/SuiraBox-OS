#include "pmm.h"

/* Initial bootstrap allocator: tracks up to 64 MiB in 4 KiB pages. */
#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_BITMAP_WORDS ((PMM_MAX_PAGES + 63u) / 64u)

static uint64_t bitmap[PMM_BITMAP_WORDS];
static uint64_t first_page;
static uint64_t page_count;
static uint64_t free_count;

static int page_in_range(uint64_t address) {
    if (address < first_page) {
        return 0;
    }
    const uint64_t index = (address - first_page) / SB_PAGE_SIZE;
    return index < page_count;
}

static void mark_used(uint64_t index) {
    bitmap[index / 64u] |= (1ull << (index % 64u));
}

static void mark_free(uint64_t index) {
    bitmap[index / 64u] &= ~(1ull << (index % 64u));
}

static int is_free(uint64_t index) {
    return (bitmap[index / 64u] & (1ull << (index % 64u))) == 0;
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    for (uint32_t i = 0; i < PMM_BITMAP_WORDS; ++i) {
        bitmap[i] = UINT64_MAX;
    }

    usable_start = (usable_start + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);
    usable_end &= ~(uint64_t)(SB_PAGE_SIZE - 1u);

    if (usable_end <= usable_start) {
        first_page = 0;
        page_count = 0;
        free_count = 0;
        return;
    }

    first_page = usable_start;
    page_count = (usable_end - usable_start) / SB_PAGE_SIZE;
    if (page_count > PMM_MAX_PAGES) {
        page_count = PMM_MAX_PAGES;
    }

    for (uint64_t i = 0; i < page_count; ++i) {
        mark_free(i);
    }
    free_count = page_count;
}

void *pmm_alloc_page(void) {
    for (uint64_t i = 0; i < page_count; ++i) {
        if (is_free(i)) {
            mark_used(i);
            --free_count;
            return (void *)(uintptr_t)(first_page + i * SB_PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free_page(void *page) {
    const uint64_t address = (uint64_t)(uintptr_t)page;
    if (!page_in_range(address) || (address - first_page) % SB_PAGE_SIZE != 0) {
        return;
    }

    const uint64_t index = (address - first_page) / SB_PAGE_SIZE;
    if (!is_free(index)) {
        mark_free(index);
        ++free_count;
    }
}

uint64_t pmm_total_pages(void) {
    return page_count;
}

uint64_t pmm_free_pages(void) {
    return free_count;
}
