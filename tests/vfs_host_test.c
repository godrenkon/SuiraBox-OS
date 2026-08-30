#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../kernel/vfs.h"

static sb_block_status_t mock_read(sb_block_device_t *device, uint64_t lba, uint32_t count, void *buffer) {
    (void)device; (void)lba; (void)count; (void)buffer; return SB_BLOCK_OK;
}

static sb_block_status_t mock_write(sb_block_device_t *device, uint64_t lba, uint32_t count, const void *buffer) {
    (void)device; (void)lba; (void)count; (void)buffer; return SB_BLOCK_OK;
}

int main(void) {
    sb_block_device_t device = {
        .name = "mock",
        .sector_count = 1024u,
        .sector_size = SB_BLOCK_SECTOR_SIZE,
        .read = mock_read,
        .write = mock_write,
        .flush = 0,
        .driver_data = 0,
    };
    sb_vfs_mount_t root = {0};
    sb_vfs_mount_t data = {0};
    char path[SB_VFS_MAX_PATH];
    char parent[SB_VFS_MAX_PATH];
    char name[SB_VFS_MAX_NAME + 1u];

    assert(sb_vfs_mount(&device, &root) == SB_VFS_OK);
    assert(sb_vfs_mount(&device, &data) == SB_VFS_OK);
    assert(sb_vfs_register_mount(&root, "/") == SB_VFS_OK);
    assert(sb_vfs_register_mount(&data, "/data/./") == SB_VFS_OK);
    assert(sb_vfs_register_mount(&root, "/") == SB_VFS_ALREADY_EXISTS);
    assert(sb_vfs_normalize_path("/data//games/./minecraft/../server", path, sizeof(path)) == SB_VFS_OK);
    assert(strcmp(path, "/data/games/server") == 0);
    assert(sb_vfs_normalize_path("../escape", path, sizeof(path)) == SB_VFS_PATH_TRAVERSAL);
    assert(sb_vfs_split_path("/data/games/server.jar", parent, sizeof(parent), name, sizeof(name)) == SB_VFS_OK);
    assert(strcmp(parent, "/data/games") == 0);
    assert(strcmp(name, "server.jar") == 0);
    assert(sb_vfs_find_mount("/data/games/server.jar") == &data);
    assert(sb_vfs_find_mount("/etc") == &root);
    assert(sb_vfs_unregister_mount(&data) == SB_VFS_OK);
    assert(sb_vfs_find_mount("/data/games/server.jar") == &root);
    assert(sb_vfs_unregister_mount(&data) == SB_VFS_NOT_FOUND);
    return 0;
}
