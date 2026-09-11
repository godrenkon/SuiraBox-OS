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
    uint64_t dirty_writes;
    uint64_t writebacks;
} sb_block_cache_stats_t;

/* Phase-1 sector cache: one 512-byte sector per entry, keyed by device
 * identity and LBA. Full-sector writes become dirty cache entries and are
 * written back on explicit flush, device unregister, or dirty replacement.
 * Devices with a different sector size bypass the cache safely. */
void sb_block_cache_reset(void);
sb_block_status_t sb_block_cache_read(sb_block_device_t *device,
                                      uint64_t lba,
                                      uint32_t count,
                                      void *buffer);
sb_block_status_t sb_block_cache_write(sb_block_device_t *device,
                                       uint64_t lba,
                                       uint32_t count,
                                       const void *buffer);
sb_block_status_t sb_block_cache_flush_device(sb_block_device_t *device);
sb_block_status_t sb_block_cache_flush_all(void);
void sb_block_cache_invalidate(sb_block_device_t *device,
                               uint64_t lba,
                               uint32_t count);
void sb_block_cache_invalidate_device(sb_block_device_t *device);
sb_block_cache_stats_t sb_block_cache_stats(void);

#endif /* SB_BLOCK_CACHE_H */
