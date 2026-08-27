#include "block.h"
#include <stdint.h>

#define SB_MAX_BLOCK_DEVICES 16u
#define SB_SELFTEST_SECTORS 8u

static sb_block_device_t *g_devices[SB_MAX_BLOCK_DEVICES];
static uint32_t g_device_count;
static uint8_t g_test_disk[SB_SELFTEST_SECTORS * SB_BLOCK_SECTOR_SIZE];

static sb_block_status_t test_disk_read(sb_block_device_t *device,
                                        uint64_t lba,
                                        uint32_t count,
                                        void *buffer) {
    if (device == 0 || buffer == 0 || count == 0 ||
        lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    uint64_t offset = lba * device->sector_size;
    uint64_t length = (uint64_t)count * device->sector_size;
    uint8_t *dst = (uint8_t *)buffer;
    for (uint64_t i = 0; i < length; ++i) {
        dst[i] = g_test_disk[offset + i];
    }
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_write(sb_block_device_t *device,
                                         uint64_t lba,
                                         uint32_t count,
                                         const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0 ||
        lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    uint64_t offset = lba * device->sector_size;
    uint64_t length = (uint64_t)count * device->sector_size;
    const uint8_t *src = (const uint8_t *)buffer;
    for (uint64_t i = 0; i < length; ++i) {
        g_test_disk[offset + i] = src[i];
    }
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_register(sb_block_device_t *device) {
    if (device == 0 || device->sector_size == 0 || device->sector_count == 0 ||
        device->read == 0 || device->write == 0 ||
        g_device_count >= SB_MAX_BLOCK_DEVICES) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    g_devices[g_device_count++] = device;
    return SB_BLOCK_OK;
}

sb_block_device_t *sb_block_get(uint32_t index) {
    if (index >= g_device_count) {
        return 0;
    }
    return g_devices[index];
}

uint32_t sb_block_count(void) {
    return g_device_count;
}

sb_block_status_t sb_block_selftest(void) {
    static sb_block_device_t test_device = {
        "memory-test",
        SB_SELFTEST_SECTORS,
        SB_BLOCK_SECTOR_SIZE,
        test_disk_read,
        test_disk_write,
        0,
    };
    static uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    static uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0xA5u);
        read_buffer[i] = 0;
    }

    if (sb_block_register(&test_device) != SB_BLOCK_OK) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    sb_block_device_t *device = sb_block_get(sb_block_count() - 1u);
    if (device == 0 || device->write(device, 2, 1, write_buffer) != SB_BLOCK_OK ||
        device->read(device, 2, 1, read_buffer) != SB_BLOCK_OK) {
        return SB_BLOCK_NOT_READY;
    }

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) {
            return SB_BLOCK_INVALID_ARGUMENT;
        }
    }

    return SB_BLOCK_OK;
}
