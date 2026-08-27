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

static uint64_t low_bits_mask(uint32_t count) {
    if (count == 0u) return 0u;
    if (count >= 64u) return UINT64_MAX;
    return (1ull << count) - 1ull;
}

static void recount_free_pages(void) {
    uint64_t count = 0u;
    for (uint32_t word = 0u; word < PMM_BITMAP_WORDS; ++word) {
        uint64_t available = ~bitmap[word];
        if (word == PMM_BITMAP_WORDS - 1u && (PMM_MAX_PAGES & 63u) != 0u) {
            available &= low_bits_mask(PMM_MAX_PAGES & 63u);
        }
        count += (uint64_t)__builtin_popcountll(available);
    }
    free_count = count;
}

static void update_range(uint64_t start, uint64_t end, int make_free) {
    const uint64_t max_address = (uint64_t)PMM_MAX_PAGES * SB_PAGE_SIZE;
    if (start >= max_address || end <= start) return;
    if (end > max_address) end = max_address;

    start = (start + SB_PAGE_SIZE - 1u) & ~(uint64_t)(SB_PAGE_SIZE - 1u);
    end &= ~(uint64_t)(SB_PAGE_SIZE - 1u);
    if (end <= start) return;

    const uint64_t first_page = start / SB_PAGE_SIZE;
    const uint64_t last_page = end / SB_PAGE_SIZE;
    const uint32_t first_word = (uint32_t)(first_page / 64u);
    const uint32_t last_word = (uint32_t)((last_page - 1u) / 64u);
    const uint32_t first_bit = (uint32_t)(first_page % 64u);
    const uint32_t last_bit = (uint32_t)((last_page - 1u) % 64u) + 1u;

    if (first_word == last_word) {
        const uint64_t mask = low_bits_mask(last_bit) & ~low_bits_mask(first_bit);
        if (make_free) bitmap[first_word] &= ~mask;
        else bitmap[first_word] |= mask;
        recount_free_pages();
        return;
    }

    if (make_free) bitmap[first_word] &= ~((uint64_t)UINT64_MAX << first_bit);
    else bitmap[first_word] |= ((uint64_t)UINT64_MAX << first_bit);

    for (uint32_t word = first_word + 1u; word < last_word; ++word) {
        bitmap[word] = make_free ? 0u : UINT64_MAX;
    }

    const uint64_t end_mask = low_bits_mask(last_bit);
    if (make_free) bitmap[last_word] &= ~end_mask;
    else bitmap[last_word] |= end_mask;

    recount_free_pages();
}

void pmm_reset(void) {
    for (uint32_t i = 0; i < PMM_BITMAP_WORDS; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    page_count = PMM_MAX_PAGES;
    free_count = 0;
}

void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end) {
    update_range(usable_start, usable_end, 1);
}

void pmm_reserve_range(uint64_t start, uint64_t end) {
    update_range(start, end, 0);
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    pmm_reset();
    pmm_add_usable_range(usable_start, usable_end);
}

void *pmm_alloc_page(void) {
    for (uint32_t word = 0u; word < PMM_BITMAP_WORDS; ++word) {
        uint64_t available = ~bitmap[word];
        if (word == PMM_BITMAP_WORDS - 1u && (PMM_MAX_PAGES & 63u) != 0u) {
            available &= low_bits_mask(PMM_MAX_PAGES & 63u);
        }
        if (available == 0u) continue;

        const uint32_t bit = (uint32_t)__builtin_ctzll(available);
        const uint64_t index = (uint64_t)word * 64u + bit;
        if (!page_index_valid(index) || !is_free(index)) continue;

        mark_used(index);
        --free_count;
        return (void *)(uintptr_t)(index * SB_PAGE_SIZE);
    }
    return 0;
}

void pmm_free_page(void *page) {
    const uint64_t address = (uint64_t)(uintptr_t)page;
    if ((address & (SB_PAGE_SIZE - 1u)) != 0u) return;

    const uint64_t index = address_to_index(address);
    if (!page_index_valid(index) || is_free(index)) return;

    mark_free(index);
    ++free_count;
}

uint64_t pmm_total_pages(void) {
    return page_count;
}

uint64_t pmm_free_pages(void) {
    return free_count;
}
