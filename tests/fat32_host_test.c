#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "block.h"
#include "vfs.h"
#include "vfs_object.h"
#include "vfs_namespace.h"
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

static sb_block_device_t device = {
    .name = "fat32-host-test-ro",
    .sector_count = TEST_SECTORS,
    .sector_size = SECTOR_SIZE,
    .read = disk_read,
    .write = 0,
    .driver_data = 0,
};

static int expect(int condition, const char *message) {
    if (condition) return 1;
    fprintf(stderr, "FAT32 test failed: %s\n", message);
    return 0;
}

static void write_83_entry(uint8_t *raw,
                           const char name[11],
                           uint8_t attributes,
                           uint32_t first_cluster,
                           uint32_t size) {
    memcpy(raw, name, 11u);
    raw[11] = attributes;
    put_le16(&raw[20], (uint16_t)(first_cluster >> 16));
    put_le16(&raw[26], (uint16_t)first_cluster);
    put_le32(&raw[28], size);
}

static void build_image(void) {
    static const char hello_name[11] = {'H','E','L','L','O',' ',' ',' ','T','X','T'};
    static const char empty_name[11] = {'E','M','P','T','Y',' ',' ',' ','T','X','T'};
    static const char games_name[11] = {'G','A','M','E','S',' ',' ',' ',' ',' ',' '};
    static const char server_name[11] = {'S','E','R','V','E','R',' ',' ','T','X','T'};
    static const char dot_name[11] = {'.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    static const char dotdot_name[11] = {'.','.',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    static const char volume_name[11] = {'S','U','I','R','A','B','O','X',' ',' ',' '};
    static const char contents[] = "Hello from SuiraBox FAT32!\n";
    static const char nested_contents[] = "Nested FAT32 file!\n";
    uint8_t *boot = &disk[0];
    uint8_t *fat = &disk[SECTOR_SIZE];
    uint8_t *root = &disk[3u * SECTOR_SIZE];
    uint8_t *file = &disk[4u * SECTOR_SIZE];
    uint8_t *games = &disk[5u * SECTOR_SIZE];
    uint8_t *nested_file = &disk[6u * SECTOR_SIZE];

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
    put_le32(&fat[8], 0x0FFFFFFFu);   /* cluster 2: root */
    put_le32(&fat[12], 0x0FFFFFFFu);  /* cluster 3: HELLO.TXT */
    put_le32(&fat[16], 0x0FFFFFFFu);  /* cluster 4: GAMES */
    put_le32(&fat[20], 0x0FFFFFFFu);  /* cluster 5: SERVER.TXT */
    memcpy(&disk[2u * SECTOR_SIZE], fat, SECTOR_SIZE);

    root[0] = 0xE5u;
    root[32] = 0x41u;
    root[32 + 11] = SB_FAT32_ATTR_LONG_NAME;
    write_83_entry(&root[64], volume_name, SB_FAT32_ATTR_VOLUME_ID, 0u, 0u);
    write_83_entry(&root[96], hello_name, 0x20u, 3u,
                   (uint32_t)(sizeof(contents) - 1u));
    write_83_entry(&root[128], empty_name, 0x20u, 0u, 0u);
    write_83_entry(&root[160], games_name, SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    root[192] = 0u;
    memcpy(file, contents, sizeof(contents) - 1u);

    write_83_entry(&games[0], dot_name, SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    write_83_entry(&games[32], dotdot_name, SB_FAT32_ATTR_DIRECTORY, 2u, 0u);
    write_83_entry(&games[64], server_name, 0x20u, 5u,
                   (uint32_t)(sizeof(nested_contents) - 1u));
    games[96] = 0u;
    memcpy(nested_file, nested_contents, sizeof(nested_contents) - 1u);
}

static int vfs_adapter_test(sb_vfs_mount_t *mount) {
    sb_fat32_vfs_t adapter;
    sb_vfs_namespace_t namespace_state;
    sb_vfs_file_t file;
    sb_vfs_file_t nested_file;
    sb_vfs_directory_t directory;
    sb_vfs_directory_t games_directory;
    sb_vfs_dir_entry_t dir_entry;
    char buffer[64] = {0};
    uint64_t bytes_read = 0u;

    if (!expect(sb_fat32_vfs_init(&adapter, mount) == SB_VFS_OBJECT_OK,
                "FAT32 VFS adapter init failed")) return 0;
    if (!expect(sb_fat32_vfs_root(&adapter) != 0,
                "FAT32 VFS root is missing")) return 0;

    sb_vfs_namespace_init(&namespace_state);
    if (!expect(sb_vfs_namespace_mount(&namespace_state,
                                       "/fat",
                                       4u,
                                       sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK,
                "FAT32 root mount failed")) return 0;

    if (!expect(sb_vfs_namespace_open_file(&namespace_state,
                                           "/fat/hello.txt",
                                           14u,
                                           SB_VFS_ACCESS_READ,
                                           &file) == SB_VFS_OBJECT_OK,
                "case-insensitive FAT32 path open failed")) return 0;
    if (!expect(sb_vfs_file_read(&file,
                                 buffer,
                                 sizeof(buffer) - 1u,
                                 &bytes_read) == SB_VFS_OBJECT_OK &&
                bytes_read == 27u &&
                strcmp(buffer, "Hello from SuiraBox FAT32!\n") == 0,
                "FAT32 generic VFS read failed")) return 0;

    memset(buffer, 0, sizeof(buffer));
    if (!expect(sb_vfs_namespace_open_file(&namespace_state,
                                           "/fat/games/server.txt",
                                           21u,
                                           SB_VFS_ACCESS_READ,
                                           &nested_file) == SB_VFS_OBJECT_OK,
                "nested FAT32 path open failed")) return 0;
    if (!expect(sb_vfs_file_read(&nested_file,
                                 buffer,
                                 sizeof(buffer) - 1u,
                                 &bytes_read) == SB_VFS_OBJECT_OK &&
                bytes_read == 19u &&
                strcmp(buffer, "Nested FAT32 file!\n") == 0,
                "nested FAT32 file read failed")) return 0;
    if (!expect(sb_vfs_file_close(&nested_file) == SB_VFS_OBJECT_OK,
                "nested FAT32 file close failed")) return 0;

    if (!expect(sb_vfs_namespace_open_directory(&namespace_state,
                                                "/fat",
                                                4u,
                                                &directory) == SB_VFS_OBJECT_OK,
                "FAT32 root directory open failed")) return 0;
    if (!expect(sb_vfs_directory_read(&directory, &dir_entry) == SB_VFS_OBJECT_OK &&
                dir_entry.type == SB_VFS_NODE_REGULAR &&
                strcmp(dir_entry.name, "HELLO.TXT") == 0 &&
                dir_entry.size == 27u,
                "FAT32 first readdir entry is wrong")) return 0;
    if (!expect(sb_vfs_directory_read(&directory, &dir_entry) == SB_VFS_OBJECT_OK &&
                dir_entry.type == SB_VFS_NODE_REGULAR &&
                strcmp(dir_entry.name, "EMPTY.TXT") == 0 &&
                dir_entry.size == 0u,
                "FAT32 second readdir entry is wrong")) return 0;
    if (!expect(sb_vfs_directory_read(&directory, &dir_entry) == SB_VFS_OBJECT_OK &&
                dir_entry.type == SB_VFS_NODE_DIRECTORY &&
                strcmp(dir_entry.name, "GAMES") == 0,
                "FAT32 directory entry is wrong")) return 0;
    if (!expect(sb_vfs_directory_read(&directory, &dir_entry) == SB_VFS_OBJECT_NOT_FOUND,
                "FAT32 directory EOF was not reported")) return 0;
    if (!expect(sb_vfs_directory_rewind(&directory) == SB_VFS_OBJECT_OK &&
                sb_vfs_directory_read(&directory, &dir_entry) == SB_VFS_OBJECT_OK &&
                strcmp(dir_entry.name, "HELLO.TXT") == 0,
                "FAT32 directory rewind failed")) return 0;
    if (!expect(sb_vfs_directory_close(&directory) == SB_VFS_OBJECT_OK,
                "FAT32 directory close failed")) return 0;

    if (!expect(sb_vfs_namespace_open_directory(&namespace_state,
                                                "/fat/GAMES",
                                                10u,
                                                &games_directory) == SB_VFS_OBJECT_OK,
                "nested FAT32 directory open failed")) return 0;
    if (!expect(sb_vfs_directory_read(&games_directory, &dir_entry) == SB_VFS_OBJECT_OK &&
                dir_entry.type == SB_VFS_NODE_REGULAR &&
                strcmp(dir_entry.name, "SERVER.TXT") == 0 &&
                dir_entry.size == 19u,
                "nested FAT32 readdir entry is wrong")) return 0;
    if (!expect(sb_vfs_directory_read(&games_directory, &dir_entry) ==
                SB_VFS_OBJECT_NOT_FOUND,
                "nested FAT32 directory EOF was not reported")) return 0;
    if (!expect(sb_vfs_directory_close(&games_directory) == SB_VFS_OBJECT_OK,
                "nested FAT32 directory close failed")) return 0;

    if (!expect(sb_vfs_namespace_unmount(&namespace_state, "/fat", 4u) ==
                SB_VFS_OBJECT_OK,
                "FAT32 namespace unmount failed")) return 0;
    if (!expect(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_ACCESS,
                "adapter destroy ignored an open file reference")) return 0;
    if (!expect(sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
                "FAT32 generic file close failed")) return 0;
    if (!expect(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK,
                "FAT32 VFS adapter destroy failed")) return 0;

    sb_vfs_namespace_destroy(&namespace_state);
    return 1;
}

int main(void) {
    sb_vfs_mount_t mount;
    sb_fat32_t fs;
    sb_fat32_dirent_t entry;
    char buffer[64];

    build_image();
    if (!expect(sb_vfs_mount(&device, &mount) == SB_VFS_OK,
                "VFS rejected a read-only block device")) return 1;
    if (!expect(sb_vfs_write_sectors(&mount, 0u, 1u, disk) == SB_VFS_READ_ONLY,
                "read-only VFS mount accepted a sector write")) return 1;
    if (!expect(sb_fat32_mount(&mount, &fs) != 0, "FAT32 mount failed")) return 1;
    if (!expect(fs.fat_count == 2u && fs.total_sectors == TEST_SECTORS,
                "BPB geometry was not retained")) return 1;

    if (!expect(sb_fat32_root_entry(&fs, 0u, &entry) == SB_FAT32_DIRENT_OK &&
                strcmp(entry.name, "HELLO.TXT") == 0 &&
                entry.first_cluster == 3u && entry.file_size == 27u,
                "first visible root entry is wrong")) return 1;

    memset(buffer, 0, sizeof(buffer));
    if (!expect(sb_fat32_read_file(&fs, &entry, 0u, entry.file_size, buffer) != 0,
                "file read failed")) return 1;
    if (!expect(strcmp(buffer, "Hello from SuiraBox FAT32!\n") == 0,
                "file contents are wrong")) return 1;
    if (!expect(sb_fat32_read_file(&fs, &entry, entry.file_size, 1u, buffer) == 0,
                "out-of-range read was accepted")) return 1;

    if (!expect(sb_fat32_root_entry(&fs, 1u, &entry) == SB_FAT32_DIRENT_OK &&
                strcmp(entry.name, "EMPTY.TXT") == 0 && entry.file_size == 0u,
                "second visible entry is wrong")) return 1;
    if (!expect(sb_fat32_read_file(&fs, &entry, 0u, 0u, buffer) != 0,
                "zero-length file read failed")) return 1;
    if (!expect(sb_fat32_root_entry(&fs, 2u, &entry) == SB_FAT32_DIRENT_OK &&
                strcmp(entry.name, "GAMES") == 0 &&
                (entry.attributes & SB_FAT32_ATTR_DIRECTORY) != 0u &&
                entry.first_cluster == 4u,
                "root subdirectory entry is wrong")) return 1;
    if (!expect(sb_fat32_root_entry(&fs, 3u, &entry) == SB_FAT32_DIRENT_END,
                "root directory end was not reported")) return 1;

    if (!expect(sb_fat32_directory_entry(&fs, 4u, 0u, &entry) ==
                SB_FAT32_DIRENT_OK &&
                strcmp(entry.name, "SERVER.TXT") == 0 &&
                entry.first_cluster == 5u && entry.file_size == 19u,
                "subdirectory parser did not skip dot entries")) return 1;
    if (!expect(sb_fat32_directory_entry(&fs, 4u, 1u, &entry) ==
                SB_FAT32_DIRENT_END,
                "subdirectory end was not reported")) return 1;

    if (!expect(sb_fat32_read_root_entry(&fs, 0u, &entry) != 0 &&
                strcmp(entry.name, "HELLO.TXT") == 0,
                "compatibility root entry wrapper failed")) return 1;

    if (!vfs_adapter_test(&mount)) return 1;

    sb_block_device_t oversized_sector_device = device;
    sb_vfs_mount_t oversized_mount;
    oversized_sector_device.sector_size = 1024u;
    oversized_sector_device.sector_count = 4u;
    if (!expect(sb_vfs_mount(&oversized_sector_device, &oversized_mount) == SB_VFS_OK,
                "generic VFS mount rejected alternate sector geometry")) return 1;
    if (!expect(sb_fat32_mount(&oversized_mount, &fs) == 0,
                "FAT32 accepted a sector size larger than its fixed buffer")) return 1;

    puts("fat32 host test OK");
    return 0;
}
