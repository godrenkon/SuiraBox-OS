#include <stdint.h>
#include "block.h"
#include "vfs.h"

#define SB_STORAGE_TEST_SECTORS 8u

static uint8_t g_test_disk[SB_STORAGE_TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];

static sb_block_status_t test_disk_read(sb_block_device_t *device,
                                        uint64_t lba,
                                        uint32_t count,
                                        void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= device->sector_count || (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    const uint64_t offset = lba * device->sector_size;
    const uint64_t length = (uint64_t)count * device->sector_size;
    for (uint64_t i = 0u; i < length; ++i) ((uint8_t *)buffer)[i] = g_test_disk[offset + i];
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_write(sb_block_device_t *device,
                                         uint64_t lba,
                                         uint32_t count,
                                         const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= device->sector_count || (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    const uint64_t offset = lba * device->sector_size;
    const uint64_t length = (uint64_t)count * device->sector_size;
    for (uint64_t i = 0u; i < length; ++i) g_test_disk[offset + i] = ((const uint8_t *)buffer)[i];
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_flush(sb_block_device_t *device) {
    return device == 0 ? SB_BLOCK_INVALID_ARGUMENT : SB_BLOCK_OK;
}

static sb_block_device_t g_test_disk_device = {
    .name = "memtest0",
    .sector_count = SB_STORAGE_TEST_SECTORS,
    .sector_size = SB_BLOCK_SECTOR_SIZE,
    .read = test_disk_read,
    .write = test_disk_write,
    .flush = test_disk_flush,
    .driver_data = 0
};

int sb_storage_selftest(void) {
    sb_vfs_mount_t mount;
    uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];

    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0x5Au);
        read_buffer[i] = 0u;
    }
    if (sb_vfs_mount(&g_test_disk_device, &mount) != SB_VFS_OK) return 0;
    if (sb_vfs_write_sectors(&mount, 2u, 1u, write_buffer) != SB_VFS_OK) return 0;
    if (sb_vfs_read_sectors(&mount, 2u, 1u, read_buffer) != SB_VFS_OK) return 0;
    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i)
        if (read_buffer[i] != write_buffer[i]) return 0;
    return g_test_disk_device.flush(&g_test_disk_device) == SB_BLOCK_OK;
}
