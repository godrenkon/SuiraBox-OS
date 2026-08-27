#include <stdint.h>
#include "block.h"
#include "vfs.h"

#define SB_STORAGE_TEST_SECTORS 8u

static uint8_t g_test_disk[SB_STORAGE_TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];

static sb_block_status_t test_disk_read(sb_block_device_t *device,
                                        uint64_t lba,
                                        uint32_t count,
                                        void *buffer) {
    uint64_t offset;
    uint32_t i;

    (void)device;
    if (buffer == 0 || count == 0 || lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    offset = lba * SB_BLOCK_SECTOR_SIZE;
    for (i = 0; i < count * SB_BLOCK_SECTOR_SIZE; ++i) {
        ((uint8_t *)buffer)[i] = g_test_disk[offset + i];
    }
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_write(sb_block_device_t *device,
                                         uint64_t lba,
                                         uint32_t count,
                                         const void *buffer) {
    uint64_t offset;
    uint32_t i;

    (void)device;
    if (buffer == 0 || count == 0 || lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    offset = lba * SB_BLOCK_SECTOR_SIZE;
    for (i = 0; i < count * SB_BLOCK_SECTOR_SIZE; ++i) {
        g_test_disk[offset + i] = ((const uint8_t *)buffer)[i];
    }
    return SB_BLOCK_OK;
}

static sb_block_device_t g_test_disk_device = {
    .name = "memtest0",
    .sector_count = SB_STORAGE_TEST_SECTORS,
    .sector_size = SB_BLOCK_SECTOR_SIZE,
    .read = test_disk_read,
    .write = test_disk_write,
    .driver_data = 0,
};

int sb_storage_selftest(void) {
    sb_vfs_mount_t mount;
    uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];
    uint32_t i;

    for (i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0x5Au);
        read_buffer[i] = 0;
    }

    if (sb_block_register(&g_test_disk_device) != SB_BLOCK_OK) {
        return 0;
    }
    if (sb_vfs_mount(&g_test_disk_device, &mount) != SB_VFS_OK) {
        return 0;
    }
    if (sb_vfs_write_sectors(&mount, 2, 1, write_buffer) != SB_VFS_OK) {
        return 0;
    }
    if (sb_vfs_read_sectors(&mount, 2, 1, read_buffer) != SB_VFS_OK) {
        return 0;
    }

    for (i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) {
            return 0;
        }
    }

    return 1;
}
