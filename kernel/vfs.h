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
    SB_VFS_NAME_TOO_LONG = 5,
    SB_VFS_PATH_INVALID = 6,
    SB_VFS_PATH_TRAVERSAL = 7,
    SB_VFS_ALREADY_EXISTS = 8,
    SB_VFS_NOT_A_DIRECTORY = 9,
} sb_vfs_status_t;

#define SB_VFS_MAX_MOUNTS 8u
#define SB_VFS_MAX_PATH 256u
#define SB_VFS_MAX_NAME 255u

typedef enum {
    SB_VFS_NODE_FILE = 1,
    SB_VFS_NODE_DIRECTORY = 2,
} sb_vfs_node_type_t;

typedef struct sb_vfs_node sb_vfs_node_t;

typedef struct {
    sb_block_device_t *block_device;
    uint32_t sector_size;
    uint64_t total_sectors;
    char mount_path[SB_VFS_MAX_PATH];
    void *filesystem;
    uint8_t mounted;
} sb_vfs_mount_t;

struct sb_vfs_node {
    sb_vfs_node_type_t type;
    uint32_t mode;
    uint64_t size;
    uint64_t inode;
    sb_vfs_mount_t *mount;
    void *private_data;
};

sb_vfs_status_t sb_vfs_mount(sb_block_device_t *device, sb_vfs_mount_t *mount);
sb_vfs_status_t sb_vfs_register_mount(sb_vfs_mount_t *mount, const char *mount_path);
sb_vfs_status_t sb_vfs_unregister_mount(sb_vfs_mount_t *mount);
sb_vfs_mount_t *sb_vfs_find_mount(const char *path);
sb_vfs_status_t sb_vfs_normalize_path(const char *input, char *output, uint32_t output_size);
sb_vfs_status_t sb_vfs_split_path(const char *path, char *parent, uint32_t parent_size,
                                   char *name, uint32_t name_size);

sb_vfs_status_t sb_vfs_read_sectors(const sb_vfs_mount_t *mount,
                                    uint64_t lba,
                                    uint32_t count,
                                    void *buffer);
sb_vfs_status_t sb_vfs_write_sectors(const sb_vfs_mount_t *mount,
                                     uint64_t lba,
                                     uint32_t count,
                                     const void *buffer);

#endif
