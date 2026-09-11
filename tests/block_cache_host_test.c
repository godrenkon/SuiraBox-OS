#include <stdint.h>
#include <stdio.h>
#include "block.h"
#include "block_cache.h"

#define TEST_SECTORS 4u

static uint8_t disk_a[TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t disk_b[TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint32_t reads_a;
static uint32_t reads_b;
static uint32_t writes_a;
static int fail_next_a_read;

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

    for (uint32_t i = 0u; i < sizeof(disk_a); ++i) disk_a[i] = (uint8_t)(i ^ 0x31u);
    for (uint32_t i = 0u; i < sizeof(disk_b); ++i) disk_b[i] = (uint8_t)(i ^ 0xA7u);
    for (uint32_t i = 0u; i < sizeof(replacement); ++i) replacement[i] = (uint8_t)(0xF0u ^ i);

    sb_block_cache_reset();
    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK,
                "first read succeeds")) return 1;
    if (require(reads_a == 1u, "first read reaches backend")) return 1;
    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK,
                "repeat read succeeds")) return 1;
    if (require(reads_a == 1u, "repeat read is cache hit")) return 1;

    sb_block_cache_stats_t stats = sb_block_cache_stats();
    if (require(stats.hits == 1u && stats.misses == 1u && stats.fills == 1u,
                "hit/miss/fill counters")) return 1;

    if (require(sb_block_read(&a, 2u, 1u, buffer) == SB_BLOCK_OK && reads_a == 2u,
                "different LBA misses")) return 1;
    if (require(sb_block_read(&b, 1u, 1u, buffer) == SB_BLOCK_OK && reads_b == 1u,
                "same LBA on another device does not alias")) return 1;

    if (require(sb_block_write(&a, 1u, 1u, replacement) == SB_BLOCK_OK,
                "write succeeds")) return 1;
    if (require(writes_a == 1u, "write reaches backend")) return 1;
    stats = sb_block_cache_stats();
    if (require(stats.invalidations == 1u, "write invalidates cached sector")) return 1;
    if (require(sb_block_read(&a, 1u, 1u, buffer) == SB_BLOCK_OK && reads_a == 3u,
                "read after write refills")) return 1;
    for (uint32_t i = 0u; i < sizeof(buffer); ++i) {
        if (require(buffer[i] == replacement[i], "refill sees written data")) return 1;
    }

    /* A backend failure is a miss, but must not create a valid cache entry. */
    sb_block_cache_invalidate(&a, 3u, 1u);
    fail_next_a_read = 1;
    const uint32_t before_failed = reads_a;
    if (require(sb_block_read(&a, 3u, 1u, buffer) == SB_BLOCK_IO_ERROR,
                "backend error propagated")) return 1;
    if (require(reads_a == before_failed + 1u, "failed read reached backend")) return 1;
    if (require(sb_block_read(&a, 3u, 1u, buffer) == SB_BLOCK_OK,
                "retry after failure succeeds")) return 1;
    if (require(reads_a == before_failed + 2u,
                "failed read did not populate cache")) return 1;

    sb_block_cache_reset();
    stats = sb_block_cache_stats();
    if (require(stats.hits == 0u && stats.misses == 0u &&
                stats.fills == 0u && stats.invalidations == 0u,
                "reset clears entries and stats")) return 1;

    /* Device lifetime owns cache-key lifetime. Unregistering must evict every
     * sector keyed by that device before the registry can reuse its slot. */
    if (require(sb_block_register(&a) == SB_BLOCK_OK &&
                sb_block_register(&b) == SB_BLOCK_OK,
                "devices register")) return 1;
    if (require(sb_block_count() == 2u && sb_block_get(0u) == &a &&
                sb_block_get(1u) == &b,
                "registry order is visible")) return 1;
    if (require(sb_block_register(&a) == SB_BLOCK_INVALID_ARGUMENT,
                "duplicate registration rejected")) return 1;

    const uint32_t before_lifecycle = reads_a;
    if (require(sb_block_read(&a, 0u, 1u, buffer) == SB_BLOCK_OK &&
                sb_block_read(&a, 0u, 1u, buffer) == SB_BLOCK_OK,
                "registered device cache fills and hits")) return 1;
    if (require(reads_a == before_lifecycle + 1u,
                "second registered-device read is cached")) return 1;

    if (require(sb_block_unregister(&a) == SB_BLOCK_OK,
                "registered device unregisters")) return 1;
    if (require(sb_block_count() == 1u && sb_block_get(0u) == &b &&
                sb_block_get(1u) == 0,
                "registry compacts after unregister")) return 1;
    stats = sb_block_cache_stats();
    if (require(stats.invalidations == 1u,
                "unregister invalidates device cache")) return 1;

    if (require(sb_block_register(&a) == SB_BLOCK_OK,
                "unregistered device can be registered again")) return 1;
    if (require(sb_block_read(&a, 0u, 1u, buffer) == SB_BLOCK_OK &&
                reads_a == before_lifecycle + 2u,
                "re-registered device cannot see stale cache entry")) return 1;

    if (require(sb_block_unregister(&b) == SB_BLOCK_OK &&
                sb_block_count() == 1u && sb_block_get(0u) == &a,
                "middle registry removal keeps remaining device")) return 1;
    if (require(sb_block_unregister(&a) == SB_BLOCK_OK && sb_block_count() == 0u,
                "registry can be emptied")) return 1;
    if (require(sb_block_unregister(&a) == SB_BLOCK_INVALID_ARGUMENT,
                "double unregister rejected")) return 1;

    puts("block cache host test OK");
    return 0;
}
