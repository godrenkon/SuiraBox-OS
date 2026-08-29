#include "storage.h"
#include "ata_pio.h"
#include "vfs.h"

static sb_vfs_mount_t g_mount;
static sb_fat32_t g_fat32;
static uint8_t g_ready;

int sb_storage_init(void) {
    sb_block_device_t *device;
    if (g_ready != 0u) return 1;

    device = sb_ata_pio_device();
    if (device == 0) {
        g_ready = 0u;
        return 0;
    }
    if (sb_vfs_mount(device, &g_mount) != SB_VFS_OK) {
        g_ready = 0u;
        return 0;
    }
    if (!sb_fat32_mount(&g_mount, &g_fat32)) {
        g_ready = 0u;
        return 0;
    }
    g_ready = 1u;
    return 1;
}

int sb_storage_ready(void) {
    return g_ready != 0u;
}

sb_fat32_t *sb_storage_fat32(void) {
    return g_ready != 0u ? &g_fat32 : 0;
}
