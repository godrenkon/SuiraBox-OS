#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "block.h"
#include "vfs.h"
#include "fs/fat32.h"

#define TEST_SECTORS 8u
#define SECTOR_SIZE 512u

static uint8_t disk[TEST_SECTORS * SECTOR_SIZE];

static uint16_t put_le16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    return value;
}

static uint32_t put_le32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
    return value;
}

static sb_block_status_t disk_read(sb_block_device_t *device, uint64_t lba,
                                   uint32_t count, void *buffer) {
    if (device == 0 || buffer == 0 || count == 0 || lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(buffer, &disk[lba * SECTOR_SIZE], (size_t)count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t disk_write(sb_block_device_t *device, uint64_t lba,
                                    uint32_t count, const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0 || lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(&disk[lba * SECTOR_SIZE], buffer, (size_t)count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_device_t device = {
    .name = "fat32-host-test",
    .sector_count = TEST_SECTORS,
    .sector_size = SECTOR_SIZE,
    .read = disk_read,
    .write = disk_write,
    .driver_data = 0,
};

static int expect(int condition, const char *message) {
    if (condition) return 1;
    fprintf(stderr, "FAT32 test failed: %s\n", message);
    return 0;
}

static void build_image(void) {
    static const char name[11] = {'H','E','L','L','O',' ',' ',' ','T','X','T'};
    static const char contents[] = "Hello from SuiraBox FAT32!\n";
    uint8_t *boot = &disk[0];
    uint8_t *fat = &disk[SECTOR_SIZE];
    uint8_t *root = &disk[3u * SECTOR_SIZE];
    uint8_t *file = &disk[4u * SECTOR_SIZE];

    memset(disk, 0, sizeof(disk));
    boot[0] = 0xEBu; boot[1] = 0x58u; boot[2] = 0x90u;
    memcpy(&boot[3], "SBOSF32 ", 8u);
    put_le16(&boot[11], SECTOR_SIZE);
    boot[13] = 1u;
    put_le16(&boot[14], 1u);
    boot[16] = 2u;
    put_le16(&boot[17], 0u);
    put_le16(&boot[19], 0u);
    boot[21] = 0xF8u;
    put_le16(&boot[22], 0u);
    put_le32(&boot[32], TEST_SECTORS);
    put_le32(&boot[36], 1u);
    put_le32(&boot[44], 2u);
    put_le16(&boot[510], 0xAA55u);

    put_le32(&fat[0], 0x0FFFFFF8u);
    put_le32(&fat[4], 0x0FFFFFFFu);
    put_le32(&fat[8], 0x0FFFFFFFu);
    put_le32(&fat[12], 0x0FFFFFFFu);
    memcpy(&disk[2u * SECTOR_SIZE], fat, SECTOR_SIZE);

    memcpy(&root[0], name, sizeof(name));
    root[11] = 0x20u;
    put_le16(&root[20], 0u);
    put_le16(&root[26], 3u);
    put_le32(&root[28], (uint32_t)(sizeof(contents) - 1u));
    root[32] = 0u;

    memcpy(file, contents, sizeof(contents) - 1u);
}

int main(void) {
    sb_vfs_mount_t mount;
    sb_fat32_t fs;
    sb_fat32_dirent_t entry;
    char buffer[64];
    static const char patch[] = "SBOS";

    build_image();
    if (!expect(sb_vfs_mount(&device, &mount) == SB_VFS_OK, "VFS mount failed")) return 1;
    if (!expect(sb_fat32_mount(&mount, &fs) != 0, "FAT32 mount failed")) return 1;
    if (!expect(sb_fat32_read_root_entry(&fs, 0u, &entry) != 0, "root entry was not found")) return 1;
    if (!expect(strcmp(entry.name, "HELLO.TXT") == 0, "8.3 filename formatting is wrong")) return 1;
    if (!expect(entry.first_cluster == 3u, "root entry cluster is wrong")) return 1;
    if (!expect(entry.file_size == 27u, "root entry file size is wrong")) return 1;

    memset(buffer, 0, sizeof(buffer));
    if (!expect(sb_fat32_read_file(&fs, &entry, 0u, entry.file_size, buffer) != 0,
                "file read failed")) return 1;
    if (!expect(strcmp(buffer, "Hello from SuiraBox FAT32!\n") == 0,
                "file contents are wrong")) return 1;

    if (!expect(sb_fat32_write_file(&fs, &entry, 0u, sizeof(patch) - 1u, patch) != 0,
                "in-place file write failed")) return 1;
    if (!expect(memcmp(&disk[4u * SECTOR_SIZE], patch, sizeof(patch) - 1u) == 0,
                "written bytes were not persisted to the sector")) return 1;
    if (!expect(disk[4u * SECTOR_SIZE + sizeof(patch) - 1u] == 'l',
                "write unexpectedly modified adjacent byte")) return 1;

    memset(buffer, 0, sizeof(buffer));
    if (!expect(sb_fat32_read_file(&fs, &entry, 0u, entry.file_size, buffer) != 0,
                "post-write file read failed")) return 1;
    if (!expect(memcmp(buffer, patch, sizeof(patch) - 1u) == 0,
                "post-write contents are wrong")) return 1;
    if (!expect(buffer[sizeof(patch) - 1u] == 'l',
                "post-write adjacent byte changed")) return 1;

    if (!expect(sb_fat32_write_file(&fs, &entry, entry.file_size, 1u, patch) == 0,
                "out-of-range write was accepted")) return 1;
    if (!expect(sb_fat32_write_file(&fs, &entry, entry.file_size - 1u, 2u, patch) == 0,
                "write extending beyond file was accepted")) return 1;
    if (!expect(sb_fat32_write_file(&fs, &entry, 0u, 0u, patch) != 0,
                "zero-length write should be a successful no-op")) return 1;
    if (!expect(sb_fat32_read_file(&fs, &entry, entry.file_size, 1u, buffer) == 0,
                "out-of-range read was accepted")) return 1;
    return 0;
}
