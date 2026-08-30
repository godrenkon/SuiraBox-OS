#include <assert.h>
#include <stdint.h>
#include "../kernel/dma.h"

static uint8_t pages[SB_DMA_MAX_PAGES][4096];
static uint32_t next_page;
static uint32_t frees;

void *pmm_alloc_page(void) {
    return next_page < SB_DMA_MAX_PAGES ? pages[next_page++] : 0;
}
void pmm_free_page(void *page) {
    assert(page != 0);
    ++frees;
}

int main(void) {
    sb_dma_region_t region;
    assert(sb_dma_alloc(0, 1u, SB_DMA_FLAG_READ) != 0);
    assert(sb_dma_alloc(&region, 0u, SB_DMA_FLAG_READ) != 0);
    assert(sb_dma_alloc(&region, 2u, SB_DMA_FLAG_READ | SB_DMA_FLAG_WRITE) == 0);
    assert(sb_dma_page_count(&region) == 2u);
    assert(sb_dma_page(&region, 0u) == pages[0]);
    assert(sb_dma_page_address(&region, 1u) == (uint64_t)(uintptr_t)pages[1]);
    assert(sb_dma_page(&region, 2u) == 0);
    sb_dma_free(&region);
    assert(sb_dma_page_count(&region) == 0u);
    assert(frees == 2u);

    next_page = SB_DMA_MAX_PAGES - 1u;
    frees = 0u;
    assert(sb_dma_alloc(&region, 2u, SB_DMA_FLAG_COHERENT) != 0);
    assert(sb_dma_page_count(&region) == 0u);
    assert(frees == 1u);
    return 0;
}
