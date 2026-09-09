#include <stdint.h>
#include "block.h"
#include "vfs.h"
#include "vfs_object.h"
#include "vfs_namespace.h"
#include "fs/fat32.h"

#define SB_STORAGE_TEST_SECTORS 8u
#define SB_STORAGE_FAT_SECTORS 8u

static uint8_t g_rw_disk[SB_STORAGE_TEST_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t g_fat_disk[SB_STORAGE_FAT_SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t g_object_data[16u];

static uint64_t text_length(const char *text) {
    uint64_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static int text_equals(const char *a, const char *b) {
    if (a == 0 || b == 0) return 0;
    uint64_t i = 0u;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return a[i] == b[i];
}

static void bytes_zero(void *buffer, uint64_t length) {
    uint8_t *dst = (uint8_t *)buffer;
    for (uint64_t i = 0u; i < length; ++i) dst[i] = 0u;
}

static void bytes_copy(void *destination, const void *source, uint64_t length) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    for (uint64_t i = 0u; i < length; ++i) dst[i] = src[i];
}

static sb_block_status_t rw_disk_read(sb_block_device_t *device,
                                      uint64_t lba,
                                      uint32_t count,
                                      void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    const uint64_t offset = lba * SB_BLOCK_SECTOR_SIZE;
    const uint64_t length = (uint64_t)count * SB_BLOCK_SECTOR_SIZE;
    bytes_copy(buffer, &g_rw_disk[offset], length);
    return SB_BLOCK_OK;
}

static sb_block_status_t rw_disk_write(sb_block_device_t *device,
                                       uint64_t lba,
                                       uint32_t count,
                                       const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= SB_STORAGE_TEST_SECTORS ||
        (uint64_t)count > SB_STORAGE_TEST_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    const uint64_t offset = lba * SB_BLOCK_SECTOR_SIZE;
    const uint64_t length = (uint64_t)count * SB_BLOCK_SECTOR_SIZE;
    bytes_copy(&g_rw_disk[offset], buffer, length);
    return SB_BLOCK_OK;
}

static sb_block_device_t g_rw_device = {
    .name = "storage-selftest-rw",
    .sector_count = SB_STORAGE_TEST_SECTORS,
    .sector_size = SB_BLOCK_SECTOR_SIZE,
    .read = rw_disk_read,
    .write = rw_disk_write,
    .driver_data = 0,
};

static int sector_io_selftest(void) {
    sb_vfs_mount_t mount;
    uint8_t write_buffer[SB_BLOCK_SECTOR_SIZE];
    uint8_t read_buffer[SB_BLOCK_SECTOR_SIZE];

    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        write_buffer[i] = (uint8_t)(i ^ 0x5Au);
        read_buffer[i] = 0u;
    }

    if (sb_block_selftest() != SB_BLOCK_OK ||
        sb_block_register(&g_rw_device) != SB_BLOCK_OK ||
        sb_vfs_mount(&g_rw_device, &mount) != SB_VFS_OK ||
        sb_vfs_write_sectors(&mount, 2u, 1u, write_buffer) != SB_VFS_OK ||
        sb_vfs_read_sectors(&mount, 2u, 1u, read_buffer) != SB_VFS_OK) {
        return 0;
    }

    for (uint32_t i = 0u; i < SB_BLOCK_SECTOR_SIZE; ++i) {
        if (read_buffer[i] != write_buffer[i]) return 0;
    }
    return 1;
}

static int object_read(sb_vfs_node_t *node,
                       uint64_t offset,
                       void *buffer,
                       uint64_t length,
                       uint64_t *bytes_read) {
    if (bytes_read != 0) *bytes_read = 0u;
    if (node == 0 || buffer == 0 || bytes_read == 0 ||
        node->private_data != g_object_data) {
        return SB_VFS_OBJECT_INVALID;
    }
    if (offset >= node->size || length == 0u) return SB_VFS_OBJECT_OK;
    uint64_t available = node->size - offset;
    if (length > available) length = available;
    bytes_copy(buffer, &g_object_data[offset], length);
    *bytes_read = length;
    return SB_VFS_OBJECT_OK;
}

static const sb_vfs_node_ops_t g_object_ops = {
    .read = object_read,
};

static int object_selftest(void) {
    sb_vfs_node_t node;
    sb_vfs_file_t file;
    uint8_t buffer[8u];
    uint64_t transferred = 0u;

    for (uint32_t i = 0u; i < sizeof(g_object_data); ++i) {
        g_object_data[i] = (uint8_t)(0x40u + i);
    }
    bytes_zero(buffer, sizeof(buffer));

    if (sb_vfs_node_init(&node,
                         SB_VFS_NODE_REGULAR,
                         SB_VFS_CAP_READ,
                         sizeof(g_object_data),
                         &g_object_ops,
                         g_object_data) != SB_VFS_OBJECT_OK ||
        sb_vfs_file_open(&node, SB_VFS_ACCESS_READ, &file) != SB_VFS_OBJECT_OK ||
        sb_vfs_file_read(&file, buffer, 4u, &transferred) != SB_VFS_OBJECT_OK ||
        transferred != 4u || file.offset != 4u ||
        buffer[0] != 0x40u || buffer[3] != 0x43u ||
        sb_vfs_file_seek(&file, 14u) != SB_VFS_OBJECT_OK ||
        sb_vfs_file_read(&file, buffer, sizeof(buffer), &transferred) !=
            SB_VFS_OBJECT_OK ||
        transferred != 2u || file.offset != 16u ||
        sb_vfs_file_close(&file) != SB_VFS_OBJECT_OK ||
        node.ref_count != 1u ||
        sb_vfs_node_release(&node) != SB_VFS_OBJECT_OK ||
        node.ref_count != 0u) {
        return 0;
    }
    return 1;
}

static void put_le16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void fat_write_83(uint8_t *raw,
                         const char name[11],
                         uint8_t attributes,
                         uint32_t first_cluster,
                         uint32_t size) {
    bytes_copy(raw, name, 11u);
    raw[11] = attributes;
    put_le16(&raw[20], (uint16_t)(first_cluster >> 16));
    put_le16(&raw[26], (uint16_t)first_cluster);
    put_le32(&raw[28], size);
}

static void build_fat_image(void) {
    static const char hello_name[11] =
        {'H','E','L','L','O',' ',' ',' ','T','X','T'};
    static const char games_name[11] =
        {'G','A','M','E','S',' ',' ',' ',' ',' ',' '};
    static const char server_name[11] =
        {'S','E','R','V','E','R',' ',' ','T','X','T'};
    static const char dot_name[11] =
        {'.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    static const char dotdot_name[11] =
        {'.','.',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    static const char hello_text[] = "Kernel FAT32 root!\n";
    static const char server_text[] = "Kernel FAT32 nested!\n";

    bytes_zero(g_fat_disk, sizeof(g_fat_disk));
    uint8_t *boot = &g_fat_disk[0u * SB_BLOCK_SECTOR_SIZE];
    uint8_t *fat = &g_fat_disk[1u * SB_BLOCK_SECTOR_SIZE];
    uint8_t *root = &g_fat_disk[3u * SB_BLOCK_SECTOR_SIZE];
    uint8_t *hello = &g_fat_disk[4u * SB_BLOCK_SECTOR_SIZE];
    uint8_t *games = &g_fat_disk[5u * SB_BLOCK_SECTOR_SIZE];
    uint8_t *server = &g_fat_disk[6u * SB_BLOCK_SECTOR_SIZE];

    boot[0] = 0xEBu; boot[1] = 0x58u; boot[2] = 0x90u;
    bytes_copy(&boot[3], "SBOSF32 ", 8u);
    put_le16(&boot[11], SB_BLOCK_SECTOR_SIZE);
    boot[13] = 1u;
    put_le16(&boot[14], 1u);
    boot[16] = 2u;
    put_le32(&boot[32], SB_STORAGE_FAT_SECTORS);
    put_le32(&boot[36], 1u);
    put_le32(&boot[44], 2u);
    put_le16(&boot[510], 0xAA55u);

    put_le32(&fat[0], 0x0FFFFFF8u);
    put_le32(&fat[4], 0x0FFFFFFFu);
    put_le32(&fat[8], 0x0FFFFFFFu);
    put_le32(&fat[12], 0x0FFFFFFFu);
    put_le32(&fat[16], 0x0FFFFFFFu);
    put_le32(&fat[20], 0x0FFFFFFFu);
    bytes_copy(&g_fat_disk[2u * SB_BLOCK_SECTOR_SIZE], fat, SB_BLOCK_SECTOR_SIZE);

    fat_write_83(&root[0], hello_name, 0x20u, 3u,
                 (uint32_t)(sizeof(hello_text) - 1u));
    fat_write_83(&root[32], games_name, SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    root[64] = 0u;
    bytes_copy(hello, hello_text, sizeof(hello_text) - 1u);

    fat_write_83(&games[0], dot_name, SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    fat_write_83(&games[32], dotdot_name, SB_FAT32_ATTR_DIRECTORY, 2u, 0u);
    fat_write_83(&games[64], server_name, 0x20u, 5u,
                 (uint32_t)(sizeof(server_text) - 1u));
    games[96] = 0u;
    bytes_copy(server, server_text, sizeof(server_text) - 1u);
}

static sb_block_status_t fat_disk_read(sb_block_device_t *device,
                                       uint64_t lba,
                                       uint32_t count,
                                       void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u ||
        lba >= SB_STORAGE_FAT_SECTORS ||
        (uint64_t)count > SB_STORAGE_FAT_SECTORS - lba) {
        return SB_BLOCK_INVALID_ARGUMENT;
    }
    const uint64_t offset = lba * SB_BLOCK_SECTOR_SIZE;
    const uint64_t length = (uint64_t)count * SB_BLOCK_SECTOR_SIZE;
    bytes_copy(buffer, &g_fat_disk[offset], length);
    return SB_BLOCK_OK;
}

static sb_block_device_t g_fat_device = {
    .name = "storage-selftest-fat-ro",
    .sector_count = SB_STORAGE_FAT_SECTORS,
    .sector_size = SB_BLOCK_SECTOR_SIZE,
    .read = fat_disk_read,
    .write = 0,
    .driver_data = 0,
};

static int fat32_vfs_selftest(void) {
    sb_vfs_mount_t mount;
    sb_fat32_vfs_t adapter;
    sb_vfs_namespace_t namespace_state;
    sb_vfs_file_t file;
    sb_vfs_directory_t directory;
    sb_vfs_dir_entry_t entry;
    uint8_t buffer[64u];
    uint64_t transferred = 0u;
    const char nested_path[] = "/fat//games/./server.txt";
    const char games_path[] = "/fat/GAMES";

    build_fat_image();
    if (sb_block_register(&g_fat_device) != SB_BLOCK_OK ||
        sb_vfs_mount(&g_fat_device, &mount) != SB_VFS_OK ||
        sb_vfs_write_sectors(&mount, 0u, 1u, g_fat_disk) != SB_VFS_READ_ONLY ||
        sb_fat32_vfs_init(&adapter, &mount) != SB_VFS_OBJECT_OK) {
        return 0;
    }

    sb_vfs_namespace_init(&namespace_state);
    if (sb_vfs_namespace_mount(&namespace_state,
                               "/fat",
                               4u,
                               sb_fat32_vfs_root(&adapter)) != SB_VFS_OBJECT_OK) {
        (void)sb_fat32_vfs_destroy(&adapter);
        return 0;
    }

    bytes_zero(buffer, sizeof(buffer));
    if (sb_vfs_namespace_open_file(&namespace_state,
                                   nested_path,
                                   text_length(nested_path),
                                   SB_VFS_ACCESS_READ,
                                   &file) != SB_VFS_OBJECT_OK ||
        sb_vfs_file_read(&file,
                         buffer,
                         sizeof(buffer) - 1u,
                         &transferred) != SB_VFS_OBJECT_OK ||
        transferred != text_length("Kernel FAT32 nested!\n") ||
        !text_equals((const char *)buffer, "Kernel FAT32 nested!\n") ||
        sb_vfs_file_close(&file) != SB_VFS_OBJECT_OK) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    if (sb_vfs_namespace_open_directory(&namespace_state,
                                        games_path,
                                        text_length(games_path),
                                        &directory) != SB_VFS_OBJECT_OK ||
        sb_vfs_directory_read(&directory, &entry) != SB_VFS_OBJECT_OK ||
        entry.type != SB_VFS_NODE_REGULAR ||
        !text_equals(entry.name, "SERVER.TXT") ||
        sb_vfs_directory_read(&directory, &entry) != SB_VFS_OBJECT_NOT_FOUND ||
        sb_vfs_directory_rewind(&directory) != SB_VFS_OBJECT_OK ||
        sb_vfs_directory_read(&directory, &entry) != SB_VFS_OBJECT_OK ||
        sb_vfs_directory_close(&directory) != SB_VFS_OBJECT_OK) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }

    if (sb_vfs_namespace_unmount(&namespace_state, "/fat", 4u) != SB_VFS_OBJECT_OK ||
        sb_fat32_vfs_destroy(&adapter) != SB_VFS_OBJECT_OK) {
        sb_vfs_namespace_destroy(&namespace_state);
        return 0;
    }
    sb_vfs_namespace_destroy(&namespace_state);
    return 1;
}

int sb_storage_selftest(void) {
    return sector_io_selftest() && object_selftest() && fat32_vfs_selftest();
}
