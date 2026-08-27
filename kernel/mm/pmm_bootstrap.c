#include "pmm.h"

#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_BITMAP_WORDS ((PMM_MAX_PAGES + 63u) / 64u)
#define PMM_FIRST_PAGE (16u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_LAST_PAGE (64u * 1024u * 1024u / SB_PAGE_SIZE)

static uint64_t bitmap[PMM_BITMAP_WORDS];
static uint64_t reserved_bitmap[PMM_BITMAP_WORDS];
static uint64_t allocated_bitmap[PMM_BITMAP_WORDS];
static uint64_t page_count;
static uint64_t free_count;

static uint32_t first_set_bit(uint64_t value) {
    uint32_t bit = 0u;
    while ((value & 1u) == 0u) { value >>= 1; ++bit; }
    return bit;
}

static void recount(void) {
    uint64_t total = 0u;
    for (uint32_t i = 0u; i < PMM_BITMAP_WORDS; ++i) {
        uint64_t available = ~bitmap[i];
        if (available == 0u) continue;
        while (available != 0u) { available &= available - 1u; ++total; }
    }
    free_count = total;
}

void pmm_reset(void) {
    for (uint32_t i = 0u; i < PMM_BITMAP_WORDS; ++i) {
        bitmap[i] = UINT64_MAX;
        reserved_bitmap[i] = 0u;
        allocated_bitmap[i] = 0u;
    }
    page_count = PMM_MAX_PAGES;
    free_count = 0u;
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    (void)usable_start;
    (void)usable_end;

    pmm_reset();

    for (uint64_t page = PMM_FIRST_PAGE; page < PMM_LAST_PAGE; ++page)
        bitmap[page >> 6] &= ~(1ull << (uint32_t)(page & 63u));

    page_count = PMM_MAX_PAGES;
    free_count = PMM_LAST_PAGE - PMM_FIRST_PAGE;
}

void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end) {
    if (usable_start < 16u * 1024u * 1024u) usable_start = 16u * 1024u * 1024u;
    if (usable_end > 64u * 1024u * 1024u) usable_end = 64u * 1024u * 1024u;
    if (usable_end <= usable_start) return;
    const uint64_t first = usable_start / SB_PAGE_SIZE;
    const uint64_t last = usable_end / SB_PAGE_SIZE;
    for (uint64_t page = first; page < last; ++page)
        bitmap[page >> 6] &= ~(1ull << (uint32_t)(page & 63u));
    recount();
}

void pmm_reserve_range(uint64_t start, uint64_t end) {
    if (start >= 64u * 1024u * 1024u || end <= start) return;
    if (end > 64u * 1024u * 1024u) end = 64u * 1024u * 1024u;
    const uint64_t first = (start + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
    const uint64_t last = end / SB_PAGE_SIZE;
    for (uint64_t page = first; page < last; ++page) {
        const uint32_t word = (uint32_t)(page >> 6);
        const uint32_t bit = (uint32_t)(page & 63u);
        const uint64_t mask = 1ull << bit;
        bitmap[word] |= mask;
        reserved_bitmap[word] |= mask;
    }
    recount();
}

void *pmm_alloc_page(void) {
    for (uint32_t word = 0u; word < PMM_BITMAP_WORDS; ++word) {
        const uint64_t available = ~bitmap[word];
        if (available == 0u) continue;
        const uint32_t bit = first_set_bit(available);
        const uint64_t index = (uint64_t)word * 64u + bit;
        if (index >= PMM_MAX_PAGES) return 0;
        const uint64_t mask = 1ull << bit;
        bitmap[word] |= mask;
        allocated_bitmap[word] |= mask;
        if (free_count != 0u) --free_count;
        return (void *)(uintptr_t)(index * SB_PAGE_SIZE);
    }
    return 0;
}

void pmm_free_page(void *page) {
    const uint64_t address = (uint64_t)(uintptr_t)page;
    if ((address & (SB_PAGE_SIZE - 1u)) != 0u) return;
    const uint64_t index = address / SB_PAGE_SIZE;
    if (index >= PMM_MAX_PAGES) return;
    const uint32_t word = (uint32_t)(index >> 6);
    const uint32_t bit = (uint32_t)(index & 63u);
    const uint64_t mask = 1ull << bit;
    if ((allocated_bitmap[word] & mask) == 0u) return;
    allocated_bitmap[word] &= ~mask;
    if ((reserved_bitmap[word] & mask) != 0u) return;
    bitmap[word] &= ~mask;
    ++free_count;
}

uint64_t pmm_total_pages(void) { return page_count; }
uint64_t pmm_free_pages(void) { return free_count; }
