#include "block.h"

#define SB_MAX_BLOCK_DEVICES 16u

static sb_block_device_t *g_devices[SB_MAX_BLOCK_DEVICES];
static uint32_t g_device_count;

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
