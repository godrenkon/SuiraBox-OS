#include <stdint.h>
#include "block.h"
#include "vfs.h"
#include "vfs_object.h"

#define SB_STORAGE_TEST_SECTORS 8u
#define SB_VFS_OBJECT_TEST_BYTES 16u

static uint8_t g_test_disk[SB_STORAGE_TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t g_object_data[SB_VFS_OBJECT_TEST_BYTES];
static uint32_t g_object_release_count;

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

    return vfs_object_selftest();
}
