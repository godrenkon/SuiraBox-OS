#include <stdint.h>
#include "block.h"
#include "vfs.h"
#include "vfs_object.h"
#include "vfs_namespace.h"

#define SB_STORAGE_TEST_SECTORS 8u
#define SB_VFS_OBJECT_TEST_BYTES 16u

static uint8_t g_test_disk[SB_STORAGE_TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t g_object_data[SB_VFS_OBJECT_TEST_BYTES];
static uint32_t g_object_release_count;

static sb_vfs_node_t g_ns_root;
static sb_vfs_node_t g_ns_games;
static sb_vfs_node_t g_ns_underlying_minecraft;
static sb_vfs_node_t g_ns_mounted_minecraft;
static sb_vfs_node_t g_ns_server;

static sb_block_status_t test_disk_read(sb_block_device_t *device,
                                        uint64_t lba,
                                        uint32_t count,
                                        void *buffer) {
    uint64_t offset;
    uint32_t i;

    (void)device;
    if (buffer == 0 || count == 0 || lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    offset = lba * SB_BLOCK_SECTOR_SIZE;
    for (i = 0; i < count * SB_BLOCK_SECTOR_SIZE; ++i) {
        ((uint8_t *)buffer)[i] = g_test_disk[offset + i];
    }
    return SB_BLOCK_OK;
}

static sb_block_status_t test_disk_write(sb_block_device_t *device,
                                         uint64_t lba,
                                         uint32_t count,
                                         const void *buffer) {
    uint64_t offset;
    uint32_t i;

    (void)device;
    if (buffer == 0 || count == 0 || lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }

    offset = lba * SB_BLOCK_SECTOR_SIZE;
    for (i = 0; i < count * SB_BLOCK_SECTOR_SIZE; ++i) {
        g_test_disk[offset + i] = ((const uint8_t *)buffer)[i];
    }
    return SB_BLOCK_OK;
}

static sb_block_device_t g_test_disk_device = {
    .name = "memtest0",
    .sector_count = SB_STORAGE_TEST_SECTORS,
    .sector_size = SB_BLOCK_SECTOR_SIZE,
    .read = test_disk_read,
    .write = test_disk_write,
    .driver_data = 0,
};

static int object_test_read(sb_vfs_node_t *node,
                            uint64_t offset,
                            void *buffer,
                            uint64_t length,
                            uint64_t *bytes_read) {
    if (node == 0 || buffer == 0 || bytes_read == 0 ||
        node->private_data != g_object_data) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (offset >= node->size) {
        *bytes_read = 0u;
        return SB_VFS_OBJECT_OK;
    }

    uint64_t available = node->size - offset;
    if (length > available) length = available;
    for (uint64_t i = 0u; i < length; ++i) {
        ((uint8_t *)buffer)[i] = g_object_data[offset + i];
    }
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static void object_test_release(sb_vfs_node_t *node) {
    if (node != 0 && node->private_data == g_object_data) ++g_object_release_count;
}

static const sb_vfs_node_ops_t g_object_test_ops = {
    .read = object_test_read,
    .write = 0,
    .lookup = 0,
    .release = object_test_release,
};

static int vfs_object_selftest(void) {
    sb_vfs_node_t node;
    sb_vfs_file_t file;
    uint8_t buffer[6] = {0};
    uint64_t transferred = 0u;

    for (uint32_t i = 0u; i < SB_VFS_OBJECT_TEST_BYTES; ++i) {
        g_object_data[i] = (uint8_t)(0x40u + i);
    }
    g_object_release_count = 0u;

    if (sb_vfs_node_init(&node,
                         SB_VFS_NODE_REGULAR,
                         SB_VFS_CAP_READ,
                         SB_VFS_OBJECT_TEST_BYTES,
                         &g_object_test_ops,
                         g_object_data) != SB_VFS_OBJECT_OK) {
        return 0;
    }
    if (sb_vfs_file_open(&node, SB_VFS_ACCESS_READ, &file) != SB_VFS_OBJECT_OK ||
        node.ref_count != 2u) {
        return 0;
    }
    if (sb_vfs_file_read(&file, buffer, 4u, &transferred) != SB_VFS_OBJECT_OK ||
        transferred != 4u || file.offset != 4u ||
        buffer[0] != 0x40u || buffer[3] != 0x43u) {
        return 0;
    }
    if (sb_vfs_file_seek(&file, SB_VFS_OBJECT_TEST_BYTES - 2u) != SB_VFS_OBJECT_OK ||
        sb_vfs_file_read(&file, buffer, sizeof(buffer), &transferred) != SB_VFS_OBJECT_OK ||
        transferred != 2u || file.offset != SB_VFS_OBJECT_TEST_BYTES) {
        return 0;
    }
    if (sb_vfs_file_seek(&file, SB_VFS_OBJECT_TEST_BYTES + 1u) != SB_VFS_OBJECT_RANGE ||
        sb_vfs_file_write(&file, buffer, 1u, &transferred) != SB_VFS_OBJECT_ACCESS) {
        return 0;
    }
    if (sb_vfs_file_close(&file) != SB_VFS_OBJECT_OK || node.ref_count != 1u ||
        sb_vfs_file_read(&file, buffer, 1u, &transferred) != SB_VFS_OBJECT_CLOSED) {
        return 0;
    }
    if (sb_vfs_node_release(&node) != SB_VFS_OBJECT_OK || node.ref_count != 0u ||
        g_object_release_count != 1u) {
        return 0;
    }
    return 1;
}

static int name_equals(const char *name,
                       uint64_t name_length,
                       const char *expected,
                       uint64_t expected_length) {
    if (name == 0 || expected == 0 || name_length != expected_length) return 0;
    for (uint64_t i = 0u; i < name_length; ++i) {
        if (name[i] != expected[i]) return 0;
    }
    return 1;
}

static int ns_root_lookup(sb_vfs_node_t *directory,
                          const char *name,
                          uint64_t name_length,
                          sb_vfs_node_t **node_out) {
    if (directory != &g_ns_root || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_equals(name, name_length, "games", 5u)) {
        *node_out = &g_ns_games;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int ns_games_lookup(sb_vfs_node_t *directory,
                           const char *name,
                           uint64_t name_length,
                           sb_vfs_node_t **node_out) {
    if (directory != &g_ns_games || node_out == 0) return SB_VFS_OBJECT_INVALID;
    if (name_equals(name, name_length, "minecraft", 9u)) {
        *node_out = &g_ns_underlying_minecraft;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int ns_mounted_lookup(sb_vfs_node_t *directory,
                             const char *name,
                             uint64_t name_length,
                             sb_vfs_node_t **node_out) {
    if (directory != &g_ns_mounted_minecraft || node_out == 0) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (name_equals(name, name_length, "server.jar", 10u)) {
        *node_out = &g_ns_server;
        return SB_VFS_OBJECT_OK;
    }
    *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static int ns_empty_lookup(sb_vfs_node_t *directory,
                           const char *name,
                           uint64_t name_length,
                           sb_vfs_node_t **node_out) {
    (void)directory;
    (void)name;
    (void)name_length;
    if (node_out != 0) *node_out = 0;
    return SB_VFS_OBJECT_NOT_FOUND;
}

static const sb_vfs_node_ops_t g_ns_root_ops = { .lookup = ns_root_lookup };
static const sb_vfs_node_ops_t g_ns_games_ops = { .lookup = ns_games_lookup };
static const sb_vfs_node_ops_t g_ns_underlying_ops = { .lookup = ns_empty_lookup };
static const sb_vfs_node_ops_t g_ns_mounted_ops = { .lookup = ns_mounted_lookup };
static const sb_vfs_node_ops_t g_ns_file_ops = {0};

static int normalized_path_equals(const char *input,
                                  uint64_t input_length,
                                  const char *expected,
                                  uint64_t expected_length) {
    char normalized[SB_VFS_PATH_MAX + 1u];
    uint64_t normalized_length = 0u;
    if (sb_vfs_path_normalize(input,
                              input_length,
                              normalized,
                              &normalized_length) != SB_VFS_OBJECT_OK ||
        normalized_length != expected_length) {
        return 0;
    }
    for (uint64_t i = 0u; i < expected_length; ++i) {
        if (normalized[i] != expected[i]) return 0;
    }
    return normalized[expected_length] == '\0';
}

static int vfs_namespace_selftest(void) {
    sb_vfs_namespace_t namespace_state;
    sb_vfs_node_t *resolved = 0;

    if (!normalized_path_equals("/games//./minecraft/tmp/../server.jar/",
                                38u,
                                "/games/minecraft/server.jar",
                                27u)) {
        return 0;
    }

    if (sb_vfs_node_init(&g_ns_root,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP,
                         0u,
                         &g_ns_root_ops,
                         0) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_init(&g_ns_games,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP,
                         0u,
                         &g_ns_games_ops,
                         0) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_init(&g_ns_underlying_minecraft,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP,
                         0u,
                         &g_ns_underlying_ops,
                         0) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_init(&g_ns_mounted_minecraft,
                         SB_VFS_NODE_DIRECTORY,
                         SB_VFS_CAP_LOOKUP,
                         0u,
                         &g_ns_mounted_ops,
                         0) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_init(&g_ns_server,
                         SB_VFS_NODE_REGULAR,
                         0u,
                         64u,
                         &g_ns_file_ops,
                         0) != SB_VFS_OBJECT_OK) {
        return 0;
    }

    sb_vfs_namespace_init(&namespace_state);
    if (sb_vfs_namespace_mount(&namespace_state, "/", 1u, &g_ns_root) !=
            SB_VFS_OBJECT_OK ||
        sb_vfs_namespace_mount(&namespace_state,
                               "/games/minecraft",
                               16u,
                               &g_ns_mounted_minecraft) != SB_VFS_OBJECT_OK ||
        namespace_state.mount_count != 2u) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/games/./minecraft/server.jar",
                                 29u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &g_ns_server) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK ||
        g_ns_server.ref_count != 1u) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    if (sb_vfs_namespace_unmount(&namespace_state,
                                 "/games/minecraft/",
                                 17u) != SB_VFS_OBJECT_OK) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    resolved = 0;
    if (sb_vfs_namespace_resolve(&namespace_state,
                                 "/games/minecraft",
                                 16u,
                                 &resolved) != SB_VFS_OBJECT_OK ||
        resolved != &g_ns_underlying_minecraft) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }
    if (sb_vfs_node_release(resolved) != SB_VFS_OBJECT_OK ||
        g_ns_underlying_minecraft.ref_count != 1u) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    sb_vfs_namespace_destroy(&namespace_state);
    if (namespace_state.mount_count != 0u ||
        g_ns_root.ref_count != 1u ||
        g_ns_mounted_minecraft.ref_count != 1u) {
        return 0;
    }

    if (sb_vfs_node_release(&g_ns_root) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&g_ns_games) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&g_ns_underlying_minecraft) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&g_ns_mounted_minecraft) != SB_VFS_OBJECT_OK ||
        sb_vfs_node_release(&g_ns_server) != SB_VFS_OBJECT_OK) {
        return 0;
    }

    return g_ns_root.ref_count == 0u &&
           g_ns_games.ref_count == 0u &&
           g_ns_underlying_minecraft.ref_count == 0u &&
           g_ns_mounted_minecraft.ref_count == 0u &&
           g_ns_server.ref_count == 0u;
}

int sb_storage_selftest(void) {
    sb_vfs_mount_t mount;
    uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];
    uint32_t i;

    for (i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0x5Au);
        read_buffer[i] = 0;
    }

    if (sb_block_register(&g_test_disk_device) != SB_BLOCK_OK) {
        return 0;
    }
    if (sb_vfs_mount(&g_test_disk_device, &mount) != SB_VFS_OK) {
        return 0;
    }
    if (sb_vfs_write_sectors(&mount, 2, 1, write_buffer) != SB_VFS_OK) {
        return 0;
    }
    if (sb_vfs_read_sectors(&mount, 2, 1, read_buffer) != SB_VFS_OK) {
        return 0;
    }

    for (i = 0; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) {
            return 0;
        }
    }

    return vfs_object_selftest() && vfs_namespace_selftest();
}
