#include "dma.h"
#include "mm/pmm.h"

int sb_dma_alloc(sb_dma_region_t *region, uint32_t page_count, uint32_t flags) {
    if (region == 0 || page_count == 0u || page_count > SB_DMA_MAX_PAGES) return -1;
    *region = (sb_dma_region_t){ .page_count = 0u, .flags = flags };
    for (uint32_t i = 0u; i < page_count; ++i) {
        void *page = pmm_alloc_page();
        if (page == 0) {
            sb_dma_free(region);
            return -2;
        }
        region->pages[region->page_count++] = page;
    }
    return 0;
}

void sb_dma_free(sb_dma_region_t *region) {
    if (region == 0) return;
    for (uint32_t i = 0u; i < region->page_count && i < SB_DMA_MAX_PAGES; ++i)
        if (region->pages[i] != 0) pmm_free_page(region->pages[i]);
    *region = (sb_dma_region_t){0};
}

void *sb_dma_page(const sb_dma_region_t *region, uint32_t index) {
    return region != 0 && index < region->page_count ? region->pages[index] : 0;
}

uint64_t sb_dma_page_address(const sb_dma_region_t *region, uint32_t index) {
    return (uint64_t)(uintptr_t)sb_dma_page(region, index);
}

uint32_t sb_dma_page_count(const sb_dma_region_t *region) {
    return region == 0 ? 0u : region->page_count;
}
