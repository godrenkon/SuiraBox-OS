#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 32u
static uint8_t disk[SECTORS * 512u], before[sizeof(disk)];
static int fail_read = -1;
static uint32_t writes, flushes;
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 recovery test failed: %s\n", why); return 1;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_read) return SB_BLOCK_IO_ERROR;
    memcpy(p, disk + lba * 512u, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, const void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    ++writes; memcpy(disk + lba * 512u, p, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++flushes; return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "recovery", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void dirent(uint32_t sector, uint32_t slot, const char *name, uint8_t attr, uint32_t cluster, uint32_t size) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attr; p[26] = (uint8_t)cluster; put32(p + 28u, size);
}
static void fixture(uint32_t status0, uint32_t status1, int active) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 1u; disk[14] = 1u; disk[16] = 2u; disk[21] = 0xF8u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    if (active) disk[40] = 0x81u;
    disk[510] = 0x55u; disk[511] = 0xAAu;
    for (uint32_t copy = 0u; copy < 2u; ++copy) {
        const uint32_t reserved = copy ? 0xB0000000u : 0xA0000000u;
        for (uint32_t c = 0u; c <= 5u; ++c)
            put32(disk + (1u + copy) * 512u + c * 4u, reserved | 0x0FFFFFFFu);
        put32(disk + (1u + copy) * 512u + 4u, reserved | (copy ? status1 : status0));
    }
    dirent(3u, 0u, "RUNTIME TXT", 0x20u, 3u, 5u);
    dirent(3u, 1u, "NEST       ", SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    dirent(5u, 0u, "INNER   TXT", 0x20u, 5u, 6u);
    memcpy(disk + 4u * 512u, "hello", 5u); memcpy(disk + 6u * 512u, "nested", 6u);
    memcpy(before, disk, sizeof(disk)); sb_block_cache_reset();
    fail_read = -1; writes = flushes = 0u;
}
int main(void) {
    sb_vfs_mount_t mount;
    sb_fat32_vfs_t adapter;
    sb_vfs_namespace_t ns;
    sb_vfs_file_t file;
    sb_vfs_directory_t dir;
    sb_vfs_dir_entry_t entry;
    sb_vfs_node_t *node;
    uint64_t n;
    uint8_t buffer[12];
    const uint32_t clean = 0x0FFFFFFFu;
    const uint32_t status[][3] = {
        {clean, clean, 0u},
        {clean & ~SB_FAT32_CLEAN_SHUTDOWN, clean & ~SB_FAT32_CLEAN_SHUTDOWN, SB_FAT32_RECOVERY_UNCLEAN},
        {clean & ~SB_FAT32_NO_HARD_ERROR, clean & ~SB_FAT32_NO_HARD_ERROR, SB_FAT32_RECOVERY_HARD_ERROR},
        {0x03FFFFFFu, 0x03FFFFFFu, SB_FAT32_RECOVERY_UNCLEAN | SB_FAT32_RECOVERY_HARD_ERROR},
        {clean, clean & ~SB_FAT32_CLEAN_SHUTDOWN, SB_FAT32_RECOVERY_UNCLEAN | SB_FAT32_RECOVERY_STATUS_MISMATCH},
        {clean & ~SB_FAT32_NO_HARD_ERROR, clean, SB_FAT32_RECOVERY_HARD_ERROR | SB_FAT32_RECOVERY_STATUS_MISMATCH},
    };
    for (uint32_t i = 0u; i < sizeof(status) / sizeof(status[0]); ++i) {
        fixture(status[i][0], status[i][1], 0);
        if (check(sb_vfs_mount(&device, &mount) == SB_VFS_OK && sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
            adapter.fs.recovery_flags == status[i][2] && writes == 0u && flushes == 0u,
            "mount inspects status copies without writes or false reserved-bit mismatch")) return 1;
        sb_vfs_namespace_init(&ns);
        if (check(sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK &&
            sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_read(&file, buffer, sizeof(buffer), &n) == SB_VFS_OBJECT_OK && n == 5u && memcmp(buffer, "hello", 5u) == 0 &&
            sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
            "clean and recovery volumes permit existing reads and read-handle sync")) return 1;
        if (status[i][2] != 0u) {
            if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
                sb_vfs_namespace_create_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
                sb_vfs_namespace_create_directory(&ns, "/disk/NEW", 9u, &dir) == SB_VFS_OBJECT_ACCESS,
                "recovery gate denies writable handles, file and directory creation")) return 1;
            if (check(sb_vfs_namespace_open_directory(&ns, "/disk/NEST", 10u, &dir) == SB_VFS_OBJECT_OK &&
                sb_vfs_directory_read(&dir, &entry) == SB_VFS_OBJECT_OK && strcmp(entry.name, "INNER.TXT") == 0 &&
                (dir.node->capabilities & (SB_VFS_CAP_CREATE | SB_VFS_CAP_MKDIR)) == 0u &&
                sb_vfs_directory_close(&dir) == SB_VFS_OBJECT_OK &&
                sb_vfs_namespace_create_file(&ns, "/disk/NEST/NEW.TXT", 18u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS,
                "nested directory iteration stays available and cached nodes deny creation")) return 1;
            if (check(sb_vfs_namespace_resolve(&ns, "/disk/RUNTIME.TXT", 17u, &node) == SB_VFS_OBJECT_OK,
                "lookup readable file for backend rejection")) return 1;
            sb_fat32_dirent_t copy = ((sb_fat32_vfs_node_t *)node->private_data)->entry;
            if (check(sb_fat32_write_file(&adapter.fs, &copy, 0u, 1u, "X", &n) == SB_VFS_OBJECT_ACCESS && n == 0u &&
                sb_vfs_node_release(node) == SB_VFS_OBJECT_OK &&
                adapter.root.ops->create(&adapter.root, "RAW.TXT", 7u, &node) == SB_VFS_OBJECT_ACCESS &&
                adapter.root.ops->mkdir(&adapter.root, "RAW", 3u, &node) == SB_VFS_OBJECT_ACCESS &&
                writes == 0u && sb_block_cache_stats().dirty_writes == 0u && memcmp(disk, before, sizeof(disk)) == 0,
                "direct backend calls cannot bypass recovery gate or clear status evidence")) return 1;
        } else {
            if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_write(&file, "H", 1u, &n) == SB_VFS_OBJECT_OK && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "clean mirrored volume remains writable")) return 1;
        }
        sb_vfs_namespace_destroy(&ns);
        if (check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "all references released")) return 1;
    }
    for (uint32_t bad = 1u; bad <= 2u; ++bad) {
        fixture(clean, clean, 0); fail_read = (int)bad;
        if (check(sb_vfs_mount(&device, &mount) == SB_VFS_OK && sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_IO &&
            !adapter.mounted && adapter.fs.mount == 0 && writes == 0u, "status read failure rejects mount without guessing")) return 1;
    }
    fixture(0x03FFFFFFu, clean, 1); fail_read = 1;
    if (check(sb_vfs_mount(&device, &mount) == SB_VFS_OK && sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        adapter.fs.recovery_flags == 0u && sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK,
        "inactive FAT status and I/O ignored when BPB selects active FAT one")) return 1;
    fixture(clean, clean & ~SB_FAT32_CLEAN_SHUTDOWN, 1);
    if (check(sb_vfs_mount(&device, &mount) == SB_VFS_OK && sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        adapter.fs.recovery_flags == SB_FAT32_RECOVERY_UNCLEAN && sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK,
        "selected active FAT determines recovery state")) return 1;
    puts("fat32 recovery host test OK"); return 0;
}
