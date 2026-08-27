#include "pmm.h"

#define PMM_MAX_PAGES (64u * 1024u * 1024u / SB_PAGE_SIZE)

static uint64_t page_count;
static uint64_t free_count;

void pmm_reset(void) {
    page_count = PMM_MAX_PAGES;
    free_count = 0u;
}

void pmm_init(uint64_t usable_start, uint64_t usable_end) {
    (void)usable_start;
    (void)usable_end;
    page_count = PMM_MAX_PAGES;
    free_count = 0u;
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
    return 0;
}

void pmm_free_page(void *page) {
    (void)page;
}

uint64_t pmm_total_pages(void) { return page_count; }
uint64_t pmm_free_pages(void) { return free_count; }
