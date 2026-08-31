#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../kernel/fs/fat32.h"

#define TEST_SECTORS 64u
#define SECTOR_SIZE 512u

static uint8_t image[TEST_SECTORS * SECTOR_SIZE];
static sb_block_device_t device;
static sb_vfs_mount_t mount;
static sb_fat32_t fs;

static sb_block_status_t read_sector(sb_block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    (void)dev;
    if (count == 0u || lba >= TEST_SECTORS || count > TEST_SECTORS - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(buffer, &image[lba * SECTOR_SIZE], count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t write_sector(sb_block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    (void)dev;
    if (count == 0u || lba >= TEST_SECTORS || count > TEST_SECTORS - lba) return SB_BLOCK_INVALID_ARGUMENT;
    memcpy(&image[lba * SECTOR_SIZE], buffer, count * SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void set_short_name(uint8_t raw[11], const char *name) {
    for (uint32_t i = 0u; i < 11u; ++i) raw[i] = ' ';
    uint32_t pos = 0u;
    for (uint32_t i = 0u; name[i] != '\0' && name[i] != '.'; ++i) raw[pos++] = name[i] >= 'a' && name[i] <= 'z' ? (uint8_t)(name[i] - 'a' + 'A') : (uint8_t)name[i];
    const char *dot = strchr(name, '.');
    if (dot != 0) {
        pos = 8u;
        for (uint32_t i = 1u; dot[i] != '\0' && i <= 3u; ++i) raw[pos++] = dot[i] >= 'a' && dot[i] <= 'z' ? (uint8_t)(dot[i] - 'a' + 'A') : (uint8_t)dot[i];
    }
}

static void write_dirent(uint32_t lba, uint32_t slot, const char *name, uint8_t attributes, uint32_t cluster, uint32_t size) {
    uint8_t *entry = &image[lba * SECTOR_SIZE + slot * 32u];
    uint8_t raw[11];
    set_short_name(raw, name);
    memset(entry, 0, 32u);
    memcpy(entry, raw, 11u);
    entry[11] = attributes;
    put16(&entry[20], (uint16_t)(cluster >> 16));
    put16(&entry[26], (uint16_t)cluster);
    put32(&entry[28], size);
}

static void setup_image(void) {
    uint8_t *boot = image;
    memset(image, 0, sizeof(image));
    put16(&boot[11], 512u);
    boot[13] = 1u;
    put16(&boot[14], 1u);
    boot[16] = 1u;
    put32(&boot[32], TEST_SECTORS);
    put32(&boot[36], 1u);
    put32(&boot[44], 2u);
    put16(&boot[510], 0xAA55u);

    uint8_t *fat = &image[SECTOR_SIZE];
    put32(&fat[0], 0x0FFFFFF8u);
    put32(&fat[4], 0xFFFFFFFFu);
    put32(&fat[8], 0x0FFFFFF8u);
    put32(&fat[12], 0x0FFFFFF8u);
    put32(&fat[16], 0x0FFFFFF8u);

    write_dirent(3u, 0u, "DATA", SB_FAT32_ATTR_DIRECTORY, 3u, 0u);
    write_dirent(3u, 1u, "ROOT.TXT", 0x20u, 4u, 4u);
    write_dirent(4u, 0u, "HELLO.TXT", 0x20u, 5u, 5u);

    memcpy(&image[5u * SECTOR_SIZE], "hello", 5u);

    device = (sb_block_device_t){
        .name = "fat32-path-test",
        .sector_count = TEST_SECTORS,
        .sector_size = SECTOR_SIZE,
        .read = read_sector,
        .write = write_sector,
        .flush = 0,
        .driver_data = 0,
    };
    mount = (sb_vfs_mount_t){0};
    assert(sb_vfs_mount(&device, &mount) == SB_VFS_OK);
    assert(sb_vfs_register_mount(&mount, "/") == SB_VFS_OK);
    assert(sb_fat32_mount(&mount, &fs) == 1);
}

int main(void) {
    sb_fat32_dirent_t entry;
    setup_image();
    assert(sb_fat32_lookup_path(&fs, "/DATA/HELLO.TXT", &entry) == 1);
    assert(strcmp(entry.name, "HELLO.TXT") == 0);
    assert(entry.file_size == 5u);
    assert(entry.first_cluster == 5u);
    assert((entry.attributes & SB_FAT32_ATTR_DIRECTORY) == 0u);
    assert(sb_fat32_lookup_path(&fs, "/DATA/MISSING.TXT", &entry) == 0);
    assert(sb_fat32_lookup_path(&fs, "/ROOT.TXT", &entry) == 0);
    assert(sb_vfs_unregister_mount(&mount) == SB_VFS_OK);
    return 0;
}
