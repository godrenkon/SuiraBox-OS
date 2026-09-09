#include "vfs.h"

static sb_vfs_status_t block_status_to_vfs(sb_block_status_t status, int writing) {
    switch (status) {
        case SB_BLOCK_OK:
            return SB_VFS_OK;
        case SB_BLOCK_INVALID_ARGUMENT:
            return SB_VFS_INVALID_ARGUMENT;
        case SB_BLOCK_NOT_READY:
            return SB_VFS_NOT_READY;
        case SB_BLOCK_UNSUPPORTED:
            return writing ? SB_VFS_READ_ONLY : SB_VFS_NOT_READY;
        case SB_BLOCK_IO_ERROR:
        default:
            return SB_VFS_IO_ERROR;
    }
}

sb_vfs_status_t sb_vfs_mount(sb_block_device_t *device, sb_vfs_mount_t *mount) {
    if (device == 0 || mount == 0 || device->read == 0 ||
        device->sector_size == 0u || device->sector_count == 0u) {
        return SB_VFS_INVALID_ARGUMENT;
    }

    mount->block_device = device;
    mount->sector_size = device->sector_size;
    mount->total_sectors = device->sector_count;
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_read_sectors(const sb_vfs_mount_t *mount,
                                     uint64_t lba,
                                     uint32_t count,
                                     void *buffer) {
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0u) {
        return SB_VFS_INVALID_ARGUMENT;
    }
    if (lba >= mount->total_sectors ||
        (uint64_t)count > mount->total_sectors - lba) {
        return SB_VFS_INVALID_ARGUMENT;
    }

    return block_status_to_vfs(sb_block_read(mount->block_device,
                                             lba,
                                             count,
                                             buffer),
                               0);
}

sb_vfs_status_t sb_vfs_write_sectors(const sb_vfs_mount_t *mount,
                                      uint64_t lba,
                                      uint32_t count,
                                      const void *buffer) {
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0u) {
        return SB_VFS_INVALID_ARGUMENT;
    }
    if (lba >= mount->total_sectors ||
        (uint64_t)count > mount->total_sectors - lba) {
        return SB_VFS_INVALID_ARGUMENT;
    }

    return block_status_to_vfs(sb_block_write(mount->block_device,
                                              lba,
                                              count,
                                              buffer),
                               1);
}
