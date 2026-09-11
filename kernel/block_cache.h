#ifndef SB_BLOCK_CACHE_H
#define SB_BLOCK_CACHE_H

#include <stdint.h>
#include "block.h"

#define SB_BLOCK_CACHE_ENTRIES 16u

typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t fills;
    uint64_t invalidations;
} sb_block_cache_stats_t;

/* Phase-1 read cache: one 512-byte sector per entry, keyed by device identity
 * and LBA. Devices with a different sector size bypass the cache safely. */
void sb_block_cache_reset(void);
sb_block_status_t sb_block_cache_read(sb_block_device_t *device,
                                      uint64_t lba,
                                      uint32_t count,
                                      void *buffer);
void sb_block_cache_invalidate(sb_block_device_t *device,
                               uint64_t lba,
                               uint32_t count);
sb_block_cache_stats_t sb_block_cache_stats(void);

#endif /* SB_BLOCK_CACHE_H */
