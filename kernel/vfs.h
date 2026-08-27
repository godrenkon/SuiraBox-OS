#ifndef SB_VFS_H
#define SB_VFS_H

#include <stdint.h>
#include "block.h"

typedef enum {
    SB_VFS_OK = 0,
    SB_VFS_INVALID_ARGUMENT = 1,
    SB_VFS_NOT_FOUND = 2,
    SB_VFS_IO_ERROR = 3,
    SB_VFS_NOT_READY = 4,
} sb_vfs_status_t;

typedef struct {
    sb_block_device_t *block_device;
    uint32_t sector_size;
    uint64_t total_sectors;
} sb_vfs_mount_t;

sb_vfs_status_t sb_vfs_mount(sb_block_device_t *device, sb_vfs_mount_t *mount);
sb_vfs_status_t sb_vfs_read_sectors(const sb_vfs_mount_t *mount,
                                     uint64_t lba,
                                     uint32_t count,
                                     void *buffer);
sb_vfs_status_t sb_vfs_write_sectors(const sb_vfs_mount_t *mount,
                                      uint64_t lba,
                                      uint32_t count,
                                      const void *buffer);

#endif
