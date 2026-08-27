#include "vfs.h"

sb_vfs_status_t sb_vfs_mount(sb_block_device_t *device, sb_vfs_mount_t *mount) {
    if (device == 0 || mount == 0 || device->read == 0 || device->write == 0 ||
        device->sector_size == 0 || device->sector_count == 0) {
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
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0) {
        return SB_VFS_INVALID_ARGUMENT;
    }
    if (lba >= mount->total_sectors ||
        (uint64_t)count > mount->total_sectors - lba) {
        return SB_VFS_INVALID_ARGUMENT;
    }

    return mount->block_device->read(mount->block_device, lba, count, buffer) == SB_BLOCK_OK
        ? SB_VFS_OK
        : SB_VFS_IO_ERROR;
}

sb_vfs_status_t sb_vfs_write_sectors(const sb_vfs_mount_t *mount,
                                     uint64_t lba,
                                     uint32_t count,
                                     const void *buffer) {
    if (mount == 0 || mount->block_device == 0 || buffer == 0 || count == 0) {
        return SB_VFS_INVALID_ARGUMENT;
    }
    if (lba >= mount->total_sectors ||
        (uint64_t)count > mount->total_sectors - lba) {
        return SB_VFS_INVALID_ARGUMENT;
    }

    return mount->block_device->write(mount->block_device, lba, count, buffer) == SB_BLOCK_OK
        ? SB_VFS_OK
        : SB_VFS_IO_ERROR;
}
