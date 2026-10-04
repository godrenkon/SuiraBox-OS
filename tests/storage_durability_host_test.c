#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "storage_durability.h"
#include "vfs.h"

static uint8_t boot[SB_BLOCK_SECTOR_SIZE];
static uint8_t volatile_sector[SB_BLOCK_SECTOR_SIZE];
static uint8_t durable_sector[SB_BLOCK_SECTOR_SIZE];
static uint32_t writes;
static uint32_t barriers;
static int fail_write;
static int fail_barrier;

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "storage durability test failed: %s\n", message);
    return 1;
}

static sb_block_status_t read_disk(sb_block_device_t *device, uint64_t lba,
                                   uint32_t count, void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || count != 1u || buffer == 0)
        return SB_BLOCK_INVALID_ARGUMENT;
    if (lba != 0u && lba != SB_DURABILITY_VOLUME_SECTORS)
        return SB_BLOCK_IO_ERROR;
    memcpy(buffer, lba == 0u ? boot : volatile_sector, SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t write_disk(sb_block_device_t *device, uint64_t lba,
                                    uint32_t count, const void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || count != 1u || buffer == 0 ||
        lba != SB_DURABILITY_VOLUME_SECTORS) return SB_BLOCK_INVALID_ARGUMENT;
    ++writes;
    if (fail_write) { fail_write = 0; return SB_BLOCK_IO_ERROR; }
    memcpy(volatile_sector, buffer, SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t flush_disk(sb_block_device_t *device) {
    if (device == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++barriers;
    if (fail_barrier) { fail_barrier = 0; return SB_BLOCK_IO_ERROR; }
    memcpy(durable_sector, volatile_sector, SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

int main(void) {
    sb_block_device_t device = {
        .name = "volatile-durability-fixture",
        .sector_count = SB_DURABILITY_VOLUME_SECTORS + 1u,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = read_disk, .write = write_disk, .flush = flush_disk,
    };
    sb_vfs_mount_t mount;
    uint8_t expected[SB_BLOCK_SECTOR_SIZE] = {0};
    uint8_t readback[SB_BLOCK_SECTOR_SIZE];
    boot[12] = 2u;
    boot[34] = 2u; /* 131072 sectors, little endian. */
    boot[510] = 0x55u; boot[511] = 0xAAu;
    memcpy(volatile_sector, SB_DURABILITY_SEED, sizeof(SB_DURABILITY_SEED) - 1u);
    memcpy(durable_sector, volatile_sector, sizeof(durable_sector));
    memcpy(expected, SB_DURABILITY_COMMIT, sizeof(SB_DURABILITY_COMMIT) - 1u);

    /* Exact geometry and a whole-sector seed are mandatory before any write. */
    if (require(!sb_storage_durability_stage(0), "null device refused")) return 1;
    device.sector_count--;
    if (require(!sb_storage_durability_stage(&device), "ordinary FAT32 disk refused")) return 1;
    device.sector_count++;
    boot[32] = 1u;
    if (require(!sb_storage_durability_stage(&device), "volume overlapping scratch refused")) return 1;
    boot[32] = 0u;
    sb_block_cache_reset();
    volatile_sector[511] = 1u;
    if (require(!sb_storage_durability_stage(&device), "corrupt seed padding refused")) return 1;
    volatile_sector[511] = 0u;
    sb_block_cache_reset();
    if (require(writes == 0u && barriers == 0u, "rejected fixtures never mutate device")) return 1;

    if (require(sb_block_register(&device) == SB_BLOCK_OK &&
                sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
                sb_storage_durability_stage(&device), "valid fixture staged")) return 1;
    if (require(writes == 0u && barriers == 0u &&
                memcmp(durable_sector, expected, sizeof(expected)) != 0,
                "staging only dirties software cache")) return 1;

    fail_write = 1;
    if (require(sb_vfs_sync(&mount) == SB_VFS_IO_ERROR && barriers == 0u,
                "failed writeback skips device barrier")) return 1;
    if (require(sb_block_read(&device, SB_DURABILITY_VOLUME_SECTORS, 1u, readback) ==
                    SB_BLOCK_OK && memcmp(readback, expected, sizeof(expected)) == 0,
                "failed writeback preserves dirty data")) return 1;

    fail_barrier = 1;
    if (require(sb_vfs_sync(&mount) == SB_VFS_IO_ERROR && writes == 2u &&
                barriers == 1u && memcmp(volatile_sector, expected, sizeof(expected)) == 0 &&
                memcmp(durable_sector, expected, sizeof(expected)) != 0,
                "device barrier failure does not claim durability")) return 1;
    if (require(sb_vfs_sync(&mount) == SB_VFS_OK && writes == 2u && barriers == 2u &&
                memcmp(durable_sector, expected, sizeof(expected)) == 0,
                "retry flushes volatile cache even after software cache is clean")) return 1;

    expected[511] = 0xA5u;
    if (require(sb_block_write(&device, SB_DURABILITY_VOLUME_SECTORS, 1u, expected) ==
                    SB_BLOCK_OK, "second update staged")) return 1;
    fail_write = 1;
    if (require(sb_block_unregister(&device) == SB_BLOCK_IO_ERROR &&
                sb_block_count() == 1u && sb_block_get(0u) == &device && barriers == 2u,
                "failed detach retains registry and skips barrier")) return 1;
    fail_barrier = 1;
    if (require(sb_block_unregister(&device) == SB_BLOCK_IO_ERROR &&
                sb_block_get(0u) == &device && barriers == 3u,
                "failed device barrier prevents detach")) return 1;
    if (require(sb_block_unregister(&device) == SB_BLOCK_OK && sb_block_count() == 0u &&
                barriers == 4u && memcmp(durable_sector, expected, sizeof(expected)) == 0,
                "successful retry commits data before detach")) return 1;
    puts("storage durability host test OK");
    return 0;
}
