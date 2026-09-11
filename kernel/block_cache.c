#include "block_cache.h"

#define SB_BLOCK_CACHE_SECTOR_BYTES SB_BLOCK_SECTOR_SIZE

typedef struct {
    sb_block_device_t *device;
    uint64_t lba;
    uint8_t data[SB_BLOCK_CACHE_SECTOR_BYTES];
    uint8_t valid;
    uint8_t dirty;
} sb_block_cache_entry_t;

static sb_block_cache_entry_t g_entries[SB_BLOCK_CACHE_ENTRIES];
static uint32_t g_replace_index;
static sb_block_cache_stats_t g_stats;

static void bytes_copy(void *destination, const void *source, uint32_t length) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    for (uint32_t i = 0u; i < length; ++i) dst[i] = src[i];
}

static void clear_entry(sb_block_cache_entry_t *entry) {
    if (entry == 0) return;
    entry->device = 0;
    entry->lba = 0u;
    entry->valid = 0u;
    entry->dirty = 0u;
}

static void invalidate_entry(sb_block_cache_entry_t *entry) {
    if (entry == 0 || entry->valid == 0u) return;
    clear_entry(entry);
    ++g_stats.invalidations;
}

static sb_block_status_t writeback_entry(sb_block_cache_entry_t *entry) {
    if (entry == 0 || entry->valid == 0u || entry->dirty == 0u) {
        return SB_BLOCK_OK;
    }
    if (entry->device == 0 || entry->device->write == 0) {
        return SB_BLOCK_UNSUPPORTED;
    }

    const sb_block_status_t status =
        entry->device->write(entry->device, entry->lba, 1u, entry->data);
    if (status != SB_BLOCK_OK) return status;

    entry->dirty = 0u;
    ++g_stats.writebacks;
    return SB_BLOCK_OK;
}

void sb_block_cache_reset(void) {
    /* Reset is intentionally a forced discard operation used during early
     * initialization/tests. Runtime device teardown must use flush+invalidate. */
    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        clear_entry(&g_entries[i]);
    }
    g_replace_index = 0u;
    g_stats = (sb_block_cache_stats_t){0};
}

static sb_block_cache_entry_t *find_entry(sb_block_device_t *device,
                                          uint64_t lba) {
    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        sb_block_cache_entry_t *entry = &g_entries[i];
        if (entry->valid != 0u && entry->device == device && entry->lba == lba) {
            return entry;
        }
    }
    return 0;
}

static sb_block_status_t acquire_entry(sb_block_device_t *device,
                                       uint64_t lba,
                                       sb_block_cache_entry_t **out_entry) {
    if (out_entry == 0) return SB_BLOCK_INVALID_ARGUMENT;

    sb_block_cache_entry_t *entry = find_entry(device, lba);
    if (entry != 0) {
        *out_entry = entry;
        return SB_BLOCK_OK;
    }

    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        if (g_entries[i].valid == 0u) {
            entry = &g_entries[i];
            entry->device = device;
            entry->lba = lba;
            entry->valid = 1u;
            entry->dirty = 0u;
            *out_entry = entry;
            return SB_BLOCK_OK;
        }
    }

    entry = &g_entries[g_replace_index];
    const sb_block_status_t status = writeback_entry(entry);
    if (status != SB_BLOCK_OK) return status;

    g_replace_index = (g_replace_index + 1u) % SB_BLOCK_CACHE_ENTRIES;
    entry->device = device;
    entry->lba = lba;
    entry->valid = 1u;
    entry->dirty = 0u;
    *out_entry = entry;
    return SB_BLOCK_OK;
}

static sb_block_status_t cached_sector_read(sb_block_device_t *device,
                                            uint64_t lba,
                                            void *buffer) {
    sb_block_cache_entry_t *entry = find_entry(device, lba);
    if (entry != 0) {
        ++g_stats.hits;
        bytes_copy(buffer, entry->data, SB_BLOCK_CACHE_SECTOR_BYTES);
        return SB_BLOCK_OK;
    }

    ++g_stats.misses;
    uint8_t sector[SB_BLOCK_CACHE_SECTOR_BYTES];
    const sb_block_status_t status = device->read(device, lba, 1u, sector);
    if (status != SB_BLOCK_OK) return status;

    status = acquire_entry(device, lba, &entry);
    if (status != SB_BLOCK_OK) return status;

    bytes_copy(entry->data, sector, SB_BLOCK_CACHE_SECTOR_BYTES);
    bytes_copy(buffer, sector, SB_BLOCK_CACHE_SECTOR_BYTES);
    ++g_stats.fills;
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_cache_read(sb_block_device_t *device,
                                      uint64_t lba,
                                      uint32_t count,
                                      void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || device->read == 0) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    if (device->sector_size != SB_BLOCK_CACHE_SECTOR_BYTES) {
        return device->read(device, lba, count, buffer);
    }

    uint8_t *dst = (uint8_t *)buffer;
    for (uint32_t i = 0u; i < count; ++i) {
        const sb_block_status_t status =
            cached_sector_read(device,
                               lba + i,
                               dst + (uint64_t)i * SB_BLOCK_CACHE_SECTOR_BYTES);
        if (status != SB_BLOCK_OK) return status;
    }
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_cache_write(sb_block_device_t *device,
                                       uint64_t lba,
                                       uint32_t count,
                                       const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || device->write == 0) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    if (device->sector_size != SB_BLOCK_CACHE_SECTOR_BYTES) {
        return device->write(device, lba, count, buffer);
    }

    const uint8_t *src = (const uint8_t *)buffer;
    for (uint32_t i = 0u; i < count; ++i) {
        sb_block_cache_entry_t *entry = 0;
        const sb_block_status_t status = acquire_entry(device, lba + i, &entry);
        if (status != SB_BLOCK_OK) return status;

        bytes_copy(entry->data,
                   src + (uint64_t)i * SB_BLOCK_CACHE_SECTOR_BYTES,
                   SB_BLOCK_CACHE_SECTOR_BYTES);
        entry->dirty = 1u;
        ++g_stats.dirty_writes;
    }
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_cache_flush_device(sb_block_device_t *device) {
    if (device == 0) return SB_BLOCK_INVALID_ARGUMENT;
    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        sb_block_cache_entry_t *entry = &g_entries[i];
        if (entry->valid == 0u || entry->device != device) continue;
        const sb_block_status_t status = writeback_entry(entry);
        if (status != SB_BLOCK_OK) return status;
    }
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_cache_flush_all(void) {
    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        const sb_block_status_t status = writeback_entry(&g_entries[i]);
        if (status != SB_BLOCK_OK) return status;
    }
    return SB_BLOCK_OK;
}

void sb_block_cache_invalidate(sb_block_device_t *device,
                               uint64_t lba,
                               uint32_t count) {
    if (device == 0 || count == 0u) return;
    const uint64_t end = lba + (uint64_t)count;
    if (end < lba) return;

    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        sb_block_cache_entry_t *entry = &g_entries[i];
        if (entry->valid == 0u || entry->device != device) continue;
        if (entry->lba >= lba && entry->lba < end) invalidate_entry(entry);
    }
}

void sb_block_cache_invalidate_device(sb_block_device_t *device) {
    if (device == 0) return;
    for (uint32_t i = 0u; i < SB_BLOCK_CACHE_ENTRIES; ++i) {
        sb_block_cache_entry_t *entry = &g_entries[i];
        if (entry->valid != 0u && entry->device == device) invalidate_entry(entry);
    }
}

sb_block_cache_stats_t sb_block_cache_stats(void) {
    return g_stats;
}
