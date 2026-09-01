#include "storage.h"
#include "ata_pio.h"
#include "vfs.h"

static sb_vfs_mount_t g_mount;
static sb_fat32_t g_fat32;
static uint8_t g_ready;

static const uint8_t g_persistence_marker[16] = {
    0x53u, 0x42u, 0x50u, 0x45u, 0x52u, 0x53u, 0x49u, 0x53u,
    0x54u, 0x45u, 0x4Eu, 0x54u, 0x2Du, 0x76u, 0x31u, 0x00u
};

static int bytes_equal(const uint8_t *a, const uint8_t *b, uint32_t length) {
    if (a == 0 || b == 0) return 0;
    for (uint32_t i = 0u; i < length; ++i)
        if (a[i] != b[i]) return 0;
    return 1;
}

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
    g_mount.filesystem = &g_fat32;
    if (sb_vfs_register_mount(&g_mount, "/") != SB_VFS_OK) {
        g_mount.filesystem = 0;
        g_ready = 0u;
        return 0;
    }
    g_ready = 1u;
    return 1;
}

int sb_storage_ready(void) { return g_ready != 0u; }
sb_fat32_t *sb_storage_fat32(void) { return g_ready != 0u ? &g_fat32 : 0; }

sb_block_status_t sb_storage_sync(void) {
    if (g_ready == 0u) return SB_BLOCK_NOT_READY;
    return sb_block_flush_all();
}

int sb_storage_persistence_selftest(void) {
    sb_fat32_dirent_t entry;
    uint8_t buffer[sizeof(g_persistence_marker)];

    if (g_ready == 0u) return -1;
    if (!sb_fat32_find_root_entry(&g_fat32, "SBPERSIS.BIN", &entry)) {
        if (!sb_fat32_create_root_file(&g_fat32, "SBPERSIS.BIN", sizeof(g_persistence_marker), &entry)) return -1;
        if (!sb_fat32_write_file(&g_fat32, &entry, 0u, sizeof(g_persistence_marker), g_persistence_marker)) return -1;
        if (sb_storage_sync() != SB_BLOCK_OK) return -1;
        return 1;
    }

    if (entry.file_size != sizeof(g_persistence_marker)) return -1;
    for (uint32_t i = 0u; i < sizeof(buffer); ++i) buffer[i] = 0u;
    if (!sb_fat32_read_file(&g_fat32, &entry, 0u, sizeof(buffer), buffer)) return -1;
    if (!bytes_equal(buffer, g_persistence_marker, sizeof(buffer))) return -1;
    return 2;
}
