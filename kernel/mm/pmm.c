#include "pmm.h"

#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_BITMAP_WORDS ((PMM_MAX_PAGES + 63u) / 64u)

/* Bootstrap allocator state. Keep it bounded and independent of firmware maps. */
static uint64_t bitmap[PMM_BITMAP_WORDS];
static uint64_t page_count;
static uint64_t free_count;

static uint64_t low_bits_mask(uint32_t count) {
    if (count == 0u) return 0u;
    if (count >= 64u) return UINT64_MAX;
    return (1ull << count) - 1ull;
}

static uint32_t count_bits64(uint64_t value) {
    uint32_t count = 0u;
    while (value != 0u) { value &= value - 1u; ++count; }
    return count;
}

static uint32_t first_set_bit64(uint64_t value) {
    uint32_t bit = 0u;
    while ((value & 1u) == 0u) { value >>= 1; ++bit; }
    return bit;
}

static void recompute_free_count(void) {
    uint64_t count = 0u;
    for (uint32_t word = 0u; word < PMM_BITMAP_WORDS; ++word) {
        uint64_t available = ~bitmap[word];
        if (word == PMM_BITMAP_WORDS - 1u && (PMM_MAX_PAGES & 63u) != 0u)
            available &= low_bits_mask(PMM_MAX_PAGES & 63u);
        count += count_bits64(available);
    }
    free_count = count;
}

static void set_range_bits(uint64_t start, uint64_t end, int make_free) {
    const uint64_t max_address = (uint64_t)PMM_MAX_PAGES * SB_PAGE_SIZE;
    uint64_t first_page;
    uint64_t last_page;
    uint32_t first_word;
    uint32_t last_word;

    if (start >= max_address || end <= start) return;
    if (end > max_address) end = max_address;

    first_page = (start + SB_PAGE_SIZE - 1u) / SB_PAGE_SIZE;
    last_page = end / SB_PAGE_SIZE;
    if (last_page <= first_page) return;

    first_word = (uint32_t)(first_page / 64u);
    last_word = (uint32_t)((last_page - 1u) / 64u);

    if (first_word == last_word) {
        const uint32_t first_bit = (uint32_t)(first_page % 64u);
        const uint32_t bit_count = (uint32_t)(last_page - first_page);
        const uint64_t mask = low_bits_mask(bit_count) << first_bit;
        if (make_free) bitmap[first_word] &= ~mask;
        else bitmap[first_word] |= mask;
        return;
    }

    if ((first_page % 64u) != 0u) {
        const uint32_t first_bit = (uint32_t)(first_page % 64u);
        const uint64_t mask = UINT64_MAX << first_bit;
        if (make_free) bitmap[first_word] &= ~mask;
        else bitmap[first_word] |= mask;
        ++first_word;
    }

    for (uint32_t word = first_word; word < last_word; ++word)
        bitmap[word] = make_free ? 0u : UINT64_MAX;

    {
        const uint32_t last_bits = (uint32_t)(last_page % 64u);
        const uint64_t mask = last_bits == 0u ? UINT64_MAX : low_bits_mask(last_bits);
        if (make_free) bitmap[last_word] &= ~mask;
        else bitmap[last_word] |= mask;
    }
}

void pmm_reset(void) {
    for (uint32_t i = 0u; i < PMM_BITMAP_WORDS; ++i) bitmap[i] = UINT64_MAX;
    page_count = PMM_MAX_PAGES;
    free_count = 0u;
}

void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end) {
    set_range_bits(usable_start, usable_end, 1);
    recompute_free_count();
}

void pmm_reserve_range(uint64_t start, uint64_t end) {
    set_range_bits(start, end, 0);
    recompute_free_count();
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    pmm_reset();
    set_range_bits(usable_start, usable_end, 1);
    recompute_free_count();
}

void *pmm_alloc_page(void) {
    for (uint32_t word = 0u; word < PMM_BITMAP_WORDS; ++word) {
        uint64_t available = ~bitmap[word];
        if (word == PMM_BITMAP_WORDS - 1u && (PMM_MAX_PAGES & 63u) != 0u)
            available &= low_bits_mask(PMM_MAX_PAGES & 63u);
        if (available == 0u) continue;
        const uint32_t bit = first_set_bit64(available);
        const uint64_t index = (uint64_t)word * 64u + bit;
        bitmap[word] |= 1ull << bit;
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
    const uint32_t word = (uint32_t)(index / 64u);
    const uint32_t bit = (uint32_t)(index % 64u);
    const uint64_t mask = 1ull << bit;
    if ((bitmap[word] & mask) == 0u) return;
    bitmap[word] &= ~mask;
    ++free_count;
}

uint64_t pmm_total_pages(void) { return page_count; }
uint64_t pmm_free_pages(void) { return free_count; }
