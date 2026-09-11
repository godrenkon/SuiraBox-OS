#include "block.h"
#include "block_cache.h"
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

sb_block_status_t sb_block_read(sb_block_device_t *device,
                                uint64_t lba,
                                uint32_t count,
                                void *buffer) {
    if (buffer == 0 || !sb_block_range_valid(device, lba, count)) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    if (device->read == 0) return SB_BLOCK_UNSUPPORTED;
    return sb_block_cache_read(device, lba, count, buffer);
}

sb_block_status_t sb_block_write(sb_block_device_t *device,
                                 uint64_t lba,
                                 uint32_t count,
                                 const void *buffer) {
    if (buffer == 0 || !sb_block_range_valid(device, lba, count)) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    if (device->write == 0) return SB_BLOCK_UNSUPPORTED;
    return sb_block_cache_write(device, lba, count, buffer);
}

sb_block_status_t sb_block_flush(sb_block_device_t *device) {
    if (device == 0) return SB_BLOCK_INVALID_ARGUMENT;
    return sb_block_cache_flush_device(device);
}

sb_block_status_t sb_block_register(sb_block_device_t *device) {
    if (device == 0 || device->name == 0 || device->sector_size == 0u ||
        device->sector_count == 0u || device->read == 0 ||
        g_device_count >= SB_MAX_BLOCK_DEVICES) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < g_device_count; ++i) {
        if (g_devices[i] == device) return SB_BLOCK_INVALID_ARGUMENT;
    }

    g_devices[g_device_count++] = device;
    return SB_BLOCK_OK;
}

sb_block_status_t sb_block_unregister(sb_block_device_t *device) {
    if (device == 0) return SB_BLOCK_INVALID_ARGUMENT;

    uint32_t index = g_device_count;
    for (uint32_t i = 0u; i < g_device_count; ++i) {
        if (g_devices[i] == device) {
            index = i;
            break;
        }
    }
    if (index == g_device_count) return SB_BLOCK_INVALID_ARGUMENT;

    /* Never detach a device while dirty cache state still depends on it. A
     * writeback failure keeps both the registry entry and dirty data intact. */
    const sb_block_status_t flush_status = sb_block_flush(device);
    if (flush_status != SB_BLOCK_OK) return flush_status;
    sb_block_cache_invalidate_device(device);

    for (uint32_t i = index + 1u; i < g_device_count; ++i) {
        g_devices[i - 1u] = g_devices[i];
    }
    --g_device_count;
    g_devices[g_device_count] = 0;
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

static sb_block_status_t selftest_finish(uint32_t initial_count,
                                         sb_block_status_t status) {
    while (g_device_count > initial_count) {
        sb_block_device_t *device = g_devices[g_device_count - 1u];
        if (sb_block_unregister(device) != SB_BLOCK_OK) {
            return SB_BLOCK_IO_ERROR;
        }
    }
    return status;
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
    static sb_block_device_t read_only_device = {
        "memory-test-ro",
        SB_SELFTEST_SECTORS,
        SB_BLOCK_SECTOR_SIZE,
        test_disk_read,
        0,
        0,
    };
    static uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    static uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];
    const uint32_t initial_count = g_device_count;

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0xA5u);
        read_buffer[i] = 0;
    }

    if (sb_block_register(&test_device) != SB_BLOCK_OK) {
        return selftest_finish(initial_count, SB_BLOCK_INVALID_ARGUMENT);
    }

    sb_block_device_t *device = sb_block_get(sb_block_count() - 1u);
    if (device == 0 || sb_block_write(device, 2u, 1u, write_buffer) != SB_BLOCK_OK ||
        sb_block_read(device, 2u, 1u, read_buffer) != SB_BLOCK_OK ||
        sb_block_flush(device) != SB_BLOCK_OK) {
        return selftest_finish(initial_count, SB_BLOCK_NOT_READY);
    }

    for (uint32_t i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) {
            return selftest_finish(initial_count, SB_BLOCK_IO_ERROR);
        }
    }

    if (sb_block_register(&read_only_device) != SB_BLOCK_OK ||
        sb_block_write(&read_only_device, 0u, 1u, write_buffer) != SB_BLOCK_UNSUPPORTED ||
        sb_block_read(&read_only_device, 2u, 1u, read_buffer) != SB_BLOCK_OK ||
        sb_block_flush(&read_only_device) != SB_BLOCK_OK) {
        return selftest_finish(initial_count, SB_BLOCK_IO_ERROR);
    }

    return selftest_finish(initial_count, SB_BLOCK_OK);
}
