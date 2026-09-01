#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "block.h"
#include "vfs.h"
#include "fs/fat32.h"

#define TEST_SECTORS 8u
#define SECTOR_SIZE 512u

static uint8_t disk[TEST_SECTORS * SECTOR_SIZE];

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

static sb_block_status_t disk_read(sb_block_device_t *device, uint64_t lba,
                                   uint32_t count, void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(buffer, &disk[lba * SECTOR_SIZE], (size_t)count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t disk_write(sb_block_device_t *device, uint64_t lba,
                                    uint32_t count, const void *buffer) {
    if (device == 0 || buffer == 0 || count == 0u || lba >= device->sector_count ||
        (uint64_t)count > device->sector_count - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(&disk[lba * SECTOR_SIZE], buffer, (size_t)count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_device_t device = {
    .name = "fat32-directory-host-test",
    .sector_count = TEST_SECTORS,
    .sector_size = SECTOR_SIZE,
    .read = disk_read,
    .write = disk_write,
    .driver_data = 0,
};

static void write_dirent(uint8_t *slot, const char name[11], uint8_t attributes,
                         uint32_t first_cluster, uint32_t file_size) {
    memset(slot, 0, 32u);
    memcpy(slot, name, 11u);
    slot[11] = attributes;
    put_le16(&slot[20], (uint16_t)(first_cluster >> 16));
    put_le16(&slot[26], (uint16_t)first_cluster);
    put_le32(&slot[28], file_size);
}

static void build_image(void) {
    static const char data_name[11] = {'D','A','T','A',' ',' ',' ',' ',' ',' ',' '};
    static const char root_file_name[11] = {'R','O','O','T',' ',' ',' ',' ','T','X','T'};
    static const char nested_name[11] = {'N','E','S','T','E','D',' ',' ','T','X','T'};
    static const char lfn_name[11] = {0x41, 'N','E','S','T','E','D',' ',' ',' ',' ',' '};
    uint8_t *boot = &disk[0u * SECTOR_SIZE];
    uint8_t *fat1 = &disk[1u * SECTOR_SIZE];
    uint8_t *fat2 = &disk[2u * SECTOR_SIZE];
    uint8_t *root = &disk[3u * SECTOR_SIZE];
    uint8_t *directory = &disk[5u * SECTOR_SIZE];

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

    for (uint32_t i = 0u; i < 2u; ++i) {
        uint8_t *fat = i == 0u ? fat1 : fat2;
        put_le32(&fat[0], 0x0FFFFFF8u);
        put_le32(&fat[4], 0x0FFFFFFFu);
        put_le32(&fat[8], 0x0FFFFFFFu);
        put_le32(&fat[12], 0x0FFFFFFFu);
        put_le32(&fat[16], 0x0FFFFFFFu);
        put_le32(&fat[20], 0x0FFFFFFFu);
    }

    write_dirent(&root[0], data_name, SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    write_dirent(&root[32], root_file_name, 0x20u, 3u, 1u);
    root[64] = 0u;

    directory[0] = 0xE5u;
    write_dirent(&directory[32], lfn_name, 0x0Fu, 0u, 0u);
    write_dirent(&directory[64], nested_name, 0x20u, 5u, 42u);
    directory[96] = 0u;
}

int main(void) {
    sb_vfs_mount_t mount;
    sb_fat32_t fs;
    sb_fat32_dirent_t entry;

    build_image();
    assert(sb_vfs_mount(&device, &mount) == SB_VFS_OK);
    assert(sb_fat32_mount(&mount, &fs) != 0);

    assert(sb_fat32_read_directory_entry(&fs, fs.root_cluster, 0u, &entry) != 0);
    assert(strcmp(entry.name, "DATA") == 0);
    assert((entry.attributes & SB_FAT32_ATTR_DIRECTORY) != 0u);
    assert(entry.first_cluster == 4u);
    assert(entry.directory_cluster == fs.root_cluster);
    assert(entry.entry_lba == 3u && entry.entry_offset == 0u);

    assert(sb_fat32_read_directory_entry(&fs, fs.root_cluster, 1u, &entry) != 0);
    assert(strcmp(entry.name, "ROOT.TXT") == 0);
    assert((entry.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u);
    assert(entry.first_cluster == 3u);
    assert(entry.entry_lba == 3u && entry.entry_offset == 32u);
    assert(sb_fat32_read_directory_entry(&fs, fs.root_cluster, 2u, &entry) == 0);

    assert(sb_fat32_read_directory_entry(&fs, 4u, 0u, &entry) != 0);
    assert(strcmp(entry.name, "NESTED.TXT") == 0);
    assert(entry.first_cluster == 5u);
    assert(entry.file_size == 42u);
    assert(entry.directory_cluster == 4u);
    assert(entry.entry_lba == 5u && entry.entry_offset == 64u);
    assert(sb_fat32_read_directory_entry(&fs, 4u, 1u, &entry) == 0);

    return 0;
}
