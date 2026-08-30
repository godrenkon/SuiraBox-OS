#ifndef SB_KERNEL_DMA_H
#define SB_KERNEL_DMA_H

#include <stdint.h>

#define SB_DMA_MAX_PAGES 64u
#define SB_DMA_FLAG_READ  0x01u
#define SB_DMA_FLAG_WRITE 0x02u
#define SB_DMA_FLAG_COHERENT 0x04u

typedef struct {
    uint32_t page_count;
    uint32_t flags;
    void *pages[SB_DMA_MAX_PAGES];
} sb_dma_region_t;

int sb_dma_alloc(sb_dma_region_t *region, uint32_t page_count, uint32_t flags);
void sb_dma_free(sb_dma_region_t *region);
void *sb_dma_page(const sb_dma_region_t *region, uint32_t index);
uint64_t sb_dma_page_address(const sb_dma_region_t *region, uint32_t index);
uint32_t sb_dma_page_count(const sb_dma_region_t *region);

#endif
