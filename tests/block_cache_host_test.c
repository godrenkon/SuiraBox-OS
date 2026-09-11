#include <stdint.h>
#include <stdio.h>
#include "block.h"
#include "block_cache.h"

#define TEST_SECTORS 20u

static uint8_t disk_a[TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t disk_b[TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint32_t reads_a;
static uint32_t reads_b;
static uint32_t writes_a;
static int fail_next_a_read;
static int fail_next_a_write;

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "block cache test failed: %s\n", message);
    return 1;
}

static void copy_bytes(void *destination, const void *source, uint64_t length) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    for (uint64_t i = 0u; i < length; ++i) dst[i] = src[i];
}

static int bytes_equal(const uint8_t *a, const uint8_t *b, uint64_t length) {
    for (uint64_t i = 0u; i < length; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static sb_block_status_t read_a(sb_block_device_t *device,
                                uint64_t lba,
                                uint32_t count,
                                void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || buffer == 0) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    ++reads_a;
    if (fail_next_a_read) {
        fail_next_a_read = 0;
        return SB_BLOCK_IO_ERROR;
    }
    copy_bytes(buffer,
               &disk_a[lba * SB_BLOCK_SECTOR_SIZE],
               (uint64_t)count * SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t write_a(sb_block_device_t *device,
                                 uint64_t lba,
                                 uint32_t count,
                                 const void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || buffer == 0) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    ++writes_a;
    if (fail_next_a_write) {
        fail_next_a_write = 0;
        return SB_BLOCK_IO_ERROR;
    }
    copy_bytes(&disk_a[lba * SB_BLOCK_SECTOR_SIZE],
               buffer,
               (uint64_t)count * SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t read_b(sb_block_device_t *device,
                                uint64_t lba,
                                uint32_t count,
                                void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || buffer == 0) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    ++reads_b;
    copy_bytes(buffer,
               &disk_b[lba * SB_BLOCK_SECTOR_SIZE],
               (uint64_t)count * SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

int main(void) {
    sb_block_device_t a = {
        .name = "cache-a",
        .sector_count = TEST_SECTORS,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = read_a,
        .write = write_a,
        .driver_data = 0,
    };
    sb_block_device_t b = {
        .name = "cache-b",
        .sector_count = TEST_SECTORS,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = read_b,
        .write = 0,
        .driver_data = 0,
    };
    uint8_t buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t replacement[SB_BLOCK_SECTOR_SIZE];
    uint8_t external[SB_BLOCK_SECTOR_SIZE];

    for (uint32_t i = 0u; i < sizeof(disk_a); ++i) disk_a[i] = (uint8_t)(i ^ 0x31u);
    for (uint32_t i = 0u; i < sizeof(disk_b); ++i) disk_b[i] = (uint8_t)(i ^ 0xA7u);
    for (uint32_t i = 0u; i < sizeof(replacement); ++i) replacement[i] = (uint8_t)(0xF0u ^ i);
    for (uint32_t i = 0u; i < sizeof(external); ++i) external[i] = (uint8_t)(0x6Du ^ i);

    sb_block_cache_reset();
    if (require(sb_block_register(&a) == SB_BLOCK_OK &&
                sb_block_register(&b) == SB_BLOCK_OK,
                "devices register")) return 1;

    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK,
                "first read succeeds")) return 1;
    if (require(reads_a == 1u, "first read reaches backend")) return 1;
    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK,
                "repeat read succeeds")) return 1;
    if (require(reads_a == 1u, "repeat read is cache hit")) return 1;

    sb_block_cache_stats_t stats = sb_block_cache_stats();
    if (require(stats.hits == 1u && stats.misses == 1u && stats.fills == 1u,
                "hit/miss/fill counters")) return 1;

    if (require(sb_block_read(&b, 1u, 1u, buffer) == SB_BLOCK_OK && reads_b == 1u,
                "same LBA on another device does not alias")) return 1;
    if (require(sb_block_write(&b, 1u, 1u, replacement) == SB_BLOCK_UNSUPPORTED,
                "read-only device rejects writes")) return 1;

    /* Full-sector writes stay dirty in cache until an explicit flush. */
    uint8_t old_backend[SB_BLOCK_SECTOR_SIZE];
    copy_bytes(old_backend, &disk_a[SB_BLOCK_SECTOR_SIZE], sizeof(old_backend));
    if (require(sb_block_write(&a, 1u, 1u, replacement) == SB_BLOCK_OK,
                "writeback write accepted")) return 1;
    if (require(writes_a == 0u, "writeback does not immediately reach backend")) return 1;
    if (require(bytes_equal(&disk_a[SB_BLOCK_SECTOR_SIZE], old_backend,
                            sizeof(old_backend)),
                "backend unchanged before flush")) return 1;
    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK &&
                bytes_equal(buffer, replacement, sizeof(buffer)),
                "read-after-write sees dirty cache data")) return 1;
    if (require(reads_a == 1u, "dirty read avoids backend")) return 1;

    stats = sb_block_cache_stats();
    if (require(stats.dirty_writes == 1u && stats.writebacks == 0u,
                "dirty write tracked before flush")) return 1;
    if (require(sb_block_cache_flush_device(&a) == SB_BLOCK_OK,
                "device flush succeeds")) return 1;
    if (require(writes_a == 1u &&
                bytes_equal(&disk_a[SB_BLOCK_SECTOR_SIZE], replacement,
                            sizeof(replacement)),
                "flush reaches backend")) return 1;
    stats = sb_block_cache_stats();
    if (require(stats.writebacks == 1u, "successful writeback counted")) return 1;

    /* Failed writeback must leave dirty data live and retryable. */
    if (require(sb_block_write(&a, 3u, 1u, external) == SB_BLOCK_OK,
                "second dirty write accepted")) return 1;
    fail_next_a_write = 1;
    if (require(sb_block_cache_flush_device(&a) == SB_BLOCK_IO_ERROR,
                "writeback error propagated")) return 1;
    if (require(sb_block_read(&a, 3u, 1u, buffer) == SB_BLOCK_OK &&
                bytes_equal(buffer, external, sizeof(buffer)),
                "dirty data survives failed writeback")) return 1;
    if (require(sb_block_cache_flush_device(&a) == SB_BLOCK_OK,
                "failed writeback can be retried")) return 1;
    if (require(bytes_equal(&disk_a[3u * SB_BLOCK_SECTOR_SIZE], external,
                            sizeof(external)),
                "retry persists dirty data")) return 1;

    /* Backend read failures are misses but never populate a valid entry. */
    sb_block_cache_invalidate(&a, 4u, 1u);
    fail_next_a_read = 1;
    const uint32_t before_failed = reads_a;
    if (require(sb_block_read(&a, 4u, 1u, buffer) == SB_BLOCK_IO_ERROR,
                "backend read error propagated")) return 1;
    if (require(reads_a == before_failed + 1u, "failed read reached backend")) return 1;
    if (require(sb_block_read(&a, 4u, 1u, buffer) == SB_BLOCK_OK,
                "retry after read failure succeeds")) return 1;
    if (require(reads_a == before_failed + 2u,
                "failed read did not populate cache")) return 1;

    /* Fill every cache slot dirty; the next write must write back the victim
     * before it can reuse the entry. */
    sb_block_cache_reset();
    const uint32_t writes_before_replacement = writes_a;
    for (uint32_t sector = 0u; sector < SB_BLOCK_CACHE_ENTRIES; ++sector) {
        replacement[0] = (uint8_t)sector;
        if (require(sb_block_write(&a, sector, 1u, replacement) == SB_BLOCK_OK,
                    "dirty cache fill succeeds")) return 1;
    }
    if (require(writes_a == writes_before_replacement,
                "dirty fill remains deferred")) return 1;
    replacement[0] = 0xEEu;
    if (require(sb_block_write(&a, SB_BLOCK_CACHE_ENTRIES, 1u, replacement) == SB_BLOCK_OK,
                "dirty replacement succeeds")) return 1;
    if (require(writes_a == writes_before_replacement + 1u,
                "dirty replacement writes victim first")) return 1;

    /* Unregister flushes every remaining dirty entry, removes the registry
     * slot and invalidates all cache keys for that device. */
    if (require(sb_block_unregister(&a) == SB_BLOCK_OK,
                "unregister flushes dirty device")) return 1;
    if (require(sb_block_count() == 1u && sb_block_get(0u) == &b,
                "registry compacts after unregister")) return 1;

    copy_bytes(&disk_a[0], external, sizeof(external));
    if (require(sb_block_register(&a) == SB_BLOCK_OK,
                "device can be registered again")) return 1;
    const uint32_t reads_before_reopen = reads_a;
    if (require(sb_block_read(&a, 0u, 1u, buffer) == SB_BLOCK_OK,
                "read after re-register succeeds")) return 1;
    if (require(reads_a == reads_before_reopen + 1u &&
                bytes_equal(buffer, external, sizeof(buffer)),
                "re-register cannot observe stale cache entry")) return 1;

    if (require(sb_block_unregister(&a) == SB_BLOCK_OK &&
                sb_block_unregister(&b) == SB_BLOCK_OK &&
                sb_block_count() == 0u,
                "all devices unregister cleanly")) return 1;

    sb_block_cache_reset();
    stats = sb_block_cache_stats();
    if (require(stats.hits == 0u && stats.misses == 0u &&
                stats.fills == 0u && stats.invalidations == 0u &&
                stats.dirty_writes == 0u && stats.writebacks == 0u,
                "reset clears entries and stats")) return 1;

    puts("block cache host test OK");
    return 0;
}
