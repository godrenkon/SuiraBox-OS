#include "storage_durability.h"

#ifndef SB_STORAGE_DURABILITY_PROOF
#error "Storage durability fixture must only be built with explicit opt-in"
#endif

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int sb_storage_durability_stage(sb_block_device_t *device) {
    static const char seed[] = SB_DURABILITY_SEED;
    static const char commit[] = SB_DURABILITY_COMMIT;
    uint8_t sector[SB_BLOCK_SECTOR_SIZE];
    const uint64_t lba = SB_DURABILITY_VOLUME_SECTORS;

    if (device == 0 || device->sector_size != SB_BLOCK_SECTOR_SIZE ||
        device->sector_count != lba + 1u || device->write == 0 ||
        device->flush == 0) return 0;
    if (sb_block_read(device, 0u, 1u, sector) != SB_BLOCK_OK ||
        sector[510] != 0x55u || sector[511] != 0xAAu ||
        sector[11] != 0u || sector[12] != 2u ||
        sector[19] != 0u || sector[20] != 0u ||
        le32(&sector[32]) != lba) return 0;
    if (sb_block_read(device, lba, 1u, sector) != SB_BLOCK_OK) return 0;

    for (uint32_t i = 0u; i < sizeof(sector); ++i) {
        const uint8_t expected = i < sizeof(seed) - 1u ? (uint8_t)seed[i] : 0u;
        if (sector[i] != expected) return 0;
    }
    for (uint32_t i = 0u; i < sizeof(sector); ++i) {
        sector[i] = i < sizeof(commit) - 1u ? (uint8_t)commit[i] : 0u;
    }
    /* Deliberately leave this dirty. Userspace FILE_SYNC on /disk/RUNTIME.TXT
     * must drive FAT32 -> VFS -> block writeback -> ATA cache flush. */
    return sb_block_write(device, lba, 1u, sector) == SB_BLOCK_OK;
}
