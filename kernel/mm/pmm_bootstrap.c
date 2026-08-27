#include "pmm.h"

#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_FIRST_PAGE (96u * 1024u * 1024u / SB_PAGE_SIZE)
#define PMM_LAST_PAGE  (120u * 1024u * 1024u / SB_PAGE_SIZE)

static uint64_t page_count;
static uint64_t free_count;
static uint64_t next_page;

void pmm_reset(void) {
    page_count = PMM_MAX_PAGES;
    next_page = PMM_FIRST_PAGE;
    free_count = PMM_LAST_PAGE - PMM_FIRST_PAGE;
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    (void)usable_start;
    (void)usable_end;
    page_count = PMM_MAX_PAGES;
    next_page = PMM_FIRST_PAGE;
    free_count = PMM_LAST_PAGE - PMM_FIRST_PAGE;
}

void pmm_add_usable_range(uint64_t usable_start, uint64_t usable_end) {
    (void)usable_start;
    (void)usable_end;
}

void pmm_reserve_range(uint64_t start, uint64_t end) {
    (void)start;
    (void)end;
}

void *pmm_alloc_page(void) {
    if (next_page >= PMM_LAST_PAGE || free_count == 0u) return 0;
    ++next_page;
    --free_count;
    return (void *)(uintptr_t)((next_page - 1u) * SB_PAGE_SIZE);
}

void pmm_free_page(void *page) {
    const uint64_t address = (uint64_t)(uintptr_t)page;
    const uint64_t first_address = (uint64_t)PMM_FIRST_PAGE * SB_PAGE_SIZE;
    const uint64_t last_address = (uint64_t)PMM_LAST_PAGE * SB_PAGE_SIZE;
    if (address < first_address || address >= last_address ||
        (address & (SB_PAGE_SIZE - 1u)) != 0u) return;
    ++free_count;
}

uint64_t pmm_total_pages(void) { return page_count; }
uint64_t pmm_free_pages(void) { return free_count; }
