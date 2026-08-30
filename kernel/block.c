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
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    const uint64_t offset = lba * device->sector_size;
    const uint64_t length = (uint64_t)count * device->sector_size;
    uint8_t *dst = (uint8_t *)buffer;
    for (uint64_t i = 0u; i < length; ++i) dst[i] = g_test_disk[offset + i];
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_write(sb_block_device_t *device,
                                         uint64_t lba,
                                         uint32_t count,
                                         const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba)
        return SB_BLOCK_INVALID_ARGUMENT;
    const uint64_t offset = lba * device->sector_size;
    const uint64_t length = (uint64_t)count * device->sector_size;
    const uint8_t *src = (const uint8_t *)buffer;
    for (uint64_t i = 0u; i < length; ++i) g_test_disk[offset + i] = src[i];
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_flush(sb_block_device_t *device) {
    return device == 0 ? SB_BLOCK_INVALID_ARGUMENT : SB_BLOCK_OK;
}

sb_block_status_t sb_block_register(sb_block_device_t *device) {
    if (device == 0 || device->sector_size == 0u || device->sector_count == 0u ||
        device->read == 0 || device->write == 0 || g_device_count >= SB_MAX_BLOCK_DEVICES)
        return SB_BLOCK_INVALID_ARGUMENT;
    g_devices[g_device_count++] = device;
    return SB_BLOCK_OK;
}

sb_block_device_t *sb_block_get(uint32_t index) {
    return index < g_device_count ? g_devices[index] : 0;
}

uint32_t sb_block_count(void) { return g_device_count; }

sb_block_status_t sb_block_flush_all(void) {
    for (uint32_t i = 0u; i < g_device_count; ++i) {
        sb_block_device_t *device = g_devices[i];
        if (device != 0 && device->flush != 0) {
            const sb_block_status_t status = device->flush(device);
            if (status != SB_BLOCK_OK) return status;
        }
    }
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_selftest(void) {
    static sb_block_device_t test_device = {
        .name = "memory-test",
        .sector_count = SB_SELFTEST_SECTORS,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = test_disk_read,
        .write = test_disk_write,
        .flush = test_disk_flush,
        .driver_data = 0
    };
    static uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    static uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];

    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0xA5u);
        read_buffer[i] = 0u;
    }
    if (sb_block_register(&test_device) != SB_BLOCK_OK) return SB_BLOCK_INVALID_ARGUMENT;
    sb_block_device_t *device = sb_block_get(sb_block_count() - 1u);
    if (device == 0 || device->write(device, 2u, 1u, write_buffer) != SB_BLOCK_OK ||
        device->read(device, 2u, 1u, read_buffer) != SB_BLOCK_OK ||
        device->flush(device) != SB_BLOCK_OK)
        return SB_BLOCK_NOT_READY;
    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i)
        if (read_buffer[i] != write_buffer[i]) return SB_BLOCK_INVALID_ARGUMENT;
    return SB_BLOCK_OK;
}
