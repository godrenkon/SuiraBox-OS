#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 64u
static uint8_t disk[SECTORS * 512u], durable[sizeof(disk)], initial[sizeof(disk)];
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_namespace_t ns;
static uint32_t barriers, fail_barrier, fail_again;
static int fail_read = -1, fail_write = -1, ordering_error;
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static uint32_t fat(const uint8_t *bytes, uint32_t copy, uint32_t cluster) {
    return get32(bytes + (3u + copy) * 512u + cluster * 4u) & 0x0FFFFFFFu;
}
static uint32_t entry_cluster(const uint8_t *p) {
    return (uint32_t)p[20] << 16 | (uint32_t)p[21] << 24 | p[26] | (uint32_t)p[27] << 8;
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 mkdir test failed: %s\n", why); return 1;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_read) { fail_read = -1; return SB_BLOCK_IO_ERROR; }
    memcpy(p, disk + lba * 512u, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, const void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_write) { fail_write = -1; return SB_BLOCK_IO_ERROR; }
    const uint8_t *bytes = p;
    if ((lba == 3u || lba == 4u) && (get32(bytes + 24u) & 0x0FFFFFFFu) >= 0x0FFFFFF8u) {
        if (barriers < 1u || memcmp(durable + 13u * 512u, ".          ", 11u) != 0 ||
            entry_cluster(durable + 13u * 512u) != 6u) ordering_error = 1;
    }
    if ((lba == 5u && memcmp(bytes + 64u, "NEWDIR     ", 11u) == 0) ||
        (lba == 15u && memcmp(bytes, "NEWDIR     ", 11u) == 0)) {
        if (barriers < 2u || fat(durable, 0u, 6u) < 0x0FFFFFF8u ||
            fat(durable, 1u, 6u) < 0x0FFFFFF8u) ordering_error = 1;
    }
    memcpy(disk + lba * 512u, p, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++barriers;
    if (barriers == fail_barrier || barriers == fail_again) return SB_BLOCK_IO_ERROR;
    memcpy(durable, disk, sizeof(disk)); return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "mkdir", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void dirent(uint32_t sector, uint32_t slot, const char *name, uint8_t attr, uint32_t cluster) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attr; p[26] = (uint8_t)cluster;
}
static int attach(void) {
    sb_vfs_namespace_init(&ns);
    return check(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
        sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK,
        "fixture mounted");
}
static int start(int full) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 2u; disk[14] = 3u; disk[16] = 2u; disk[48] = 1u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    put32(disk + 512u, 0x41615252u); put32(disk + 512u + 484u, 0x61417272u);
    put32(disk + 512u + 508u, 0xAA550000u);
    put32(disk + 512u + 488u, 20u); put32(disk + 512u + 492u, 6u);
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 0u; c < 128u; ++c)
            put32(disk + (3u + copy) * 512u + c * 4u, (copy ? 0xB0000000u : 0xA0000000u) |
                (c < 6u ? (c == 2u ? 3u : 0x0FFFFFFFu) : 0u));
    dirent(5u, 0u, "NEST       ", SB_FAT32_ATTR_DIRECTORY, 4u);
    dirent(5u, 1u, "LOCKED     ", SB_FAT32_ATTR_DIRECTORY | SB_FAT32_ATTR_READ_ONLY, 5u);
    if (full) for (uint32_t i = 2u; i < 64u; ++i) {
        char name[12] = "F000    TXT";
        name[2] = (char)('0' + i / 10u); name[3] = (char)('0' + i % 10u);
        dirent(5u + i / 16u, i % 16u, name, 0x20u, 0u);
    }
    memset(disk + 13u * 512u, 0xCC, 1024u);
    memcpy(initial, disk, sizeof(disk)); memcpy(durable, disk, sizeof(disk));
    sb_block_cache_reset(); barriers = fail_barrier = fail_again = 0u;
    fail_read = fail_write = -1; ordering_error = 0;
    if (attach()) return 1;
    /* Dedicated dirty-session tests cover the first barrier; these target the
     * later data/FAT/publication phases. */
    if (check(sb_fat32_begin_write(&adapter.fs) == SB_VFS_OBJECT_OK, "dirty session prepared")) return 1;
    memcpy(initial, disk, sizeof(disk)); sb_block_cache_reset(); barriers = 0u;
    return 0;
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "references released");
}
static int mkdir_path(const char *path, sb_vfs_directory_t *dir) {
    return sb_vfs_namespace_create_directory(&ns, path, strlen(path), dir);
}
int main(void) {
    sb_vfs_directory_t dir, nested;
    sb_vfs_dir_entry_t entry;
    sb_vfs_file_t file;
    uint64_t n;
    uint8_t buffer[12];
    if (start(0)) return 1;
    if (check(mkdir_path("/disk/newdir", &dir) == SB_VFS_OBJECT_OK && barriers == 3u && !ordering_error &&
        dir.node->type == SB_VFS_NODE_DIRECTORY && dir.node->size == 0u &&
        sb_vfs_directory_read(&dir, &entry) == SB_VFS_OBJECT_NOT_FOUND,
        "durable directory returned with hidden dot entries")) return 1;
    const uint8_t *child = durable + 13u * 512u;
    if (check(memcmp(child, ".          ", 11u) == 0 && child[11] == SB_FAT32_ATTR_DIRECTORY &&
        entry_cluster(child) == 6u && memcmp(child + 32u, "..         ", 11u) == 0 &&
        entry_cluster(child + 32u) == 0u && fat(durable, 0u, 6u) >= 0x0FFFFFF8u &&
        fat(durable, 1u, 6u) >= 0x0FFFFFF8u && memcmp(durable + 5u * 512u, initial + 5u * 512u, 64u) == 0,
        "root-parent dotdot, mirrored allocation, and existing entries preserved")) return 1;
    for (uint32_t i = 64u; i < 1024u; ++i)
        if (check(child[i] == 0u, "all sectors and unused entries zeroed")) return 1;
    const uint64_t dirty = sb_block_cache_stats().dirty_writes;
    if (check(mkdir_path("/disk/NEWDIR", &nested) == SB_VFS_OBJECT_EXISTS && !nested.open &&
        sb_block_cache_stats().dirty_writes == dirty, "duplicate does not truncate or allocate")) return 1;
    if (check(mkdir_path("/disk/NEWDIR/INNER", &nested) == SB_VFS_OBJECT_OK &&
        entry_cluster(durable + 15u * 512u + 32u) == 6u &&
        sb_vfs_directory_close(&nested) == SB_VFS_OBJECT_OK,
        "nested dotdot points to actual parent cluster")) return 1;
    if (check(sb_vfs_namespace_create_file(&ns, "/disk/NEWDIR/INNER/LEVEL.DAT", 28u,
        SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, "world-data", 10u, &n) == SB_VFS_OBJECT_OK && n == 10u &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_directory_read(&dir, &entry) == SB_VFS_OBJECT_OK && entry.type == SB_VFS_NODE_DIRECTORY &&
        strcmp(entry.name, "INNER") == 0 && sb_vfs_directory_close(&dir) == SB_VFS_OBJECT_OK,
        "new directory accepts nested files and enumeration")) return 1;
    if (finish()) return 1;
    sb_block_cache_reset(); memcpy(disk, durable, sizeof(disk));
    if (attach()) return 1;
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/NEWDIR/INNER/LEVEL.DAT", 28u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&file, buffer, sizeof(buffer), &n) == SB_VFS_OBJECT_OK && n == 10u &&
        memcmp(buffer, "world-data", 10u) == 0 && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
        "nested files survive remount and cache loss")) return 1;
    if (finish()) return 1;
    for (uint32_t fault = 0u; fault < 5u; ++fault) {
        if (start(0)) return 1;
        if (fault == 0u) fail_write = 13;
        if (fault == 1u) fail_barrier = 1u;
        if (fault == 2u) fail_write = 4;
        if (fault == 3u) fail_barrier = 2u;
        if (fault == 4u) fail_read = 1;
        if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_IO && !dir.open && !adapter.fs.write_faulted &&
            fat(durable, 0u, 6u) == 0u && fat(durable, 1u, 6u) == 0u &&
            memcmp(durable + 5u * 512u, initial + 5u * 512u, 4u * 512u) == 0,
            "early failure leaves parent untouched and frees child allocation")) return 1;
        if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_OK && !ordering_error &&
            sb_vfs_directory_close(&dir) == SB_VFS_OBJECT_OK, "early failure can retry")) return 1;
        if (finish()) return 1;
    }
    for (uint32_t fault = 0u; fault < 3u; ++fault) {
        if (start(0)) return 1;
        if (fault == 0u) fail_barrier = 3u;
        if (fault == 1u) fail_write = 5;
        if (fault == 2u) { fail_barrier = 2u; fail_again = 3u; }
        if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_IO && !dir.open && adapter.fs.write_faulted &&
            mkdir_path("/disk/OTHER", &dir) == SB_VFS_OBJECT_IO,
            "uncertain publication or failed rollback quarantines writes")) return 1;
        if (fault != 2u && check(fat(durable, 0u, 6u) >= 0x0FFFFFF8u && fat(durable, 1u, 6u) >= 0x0FFFFFF8u,
            "uncertain parent publication retains a valid child allocation")) return 1;
        if (finish()) return 1;
    }
    if (start(1)) return 1;
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_OK && !ordering_error && barriers == 5u &&
        fat(durable, 0u, 3u) == 7u && fat(durable, 1u, 3u) == 7u &&
        entry_cluster(durable + 15u * 512u) == 6u &&
        memcmp(durable + 5u * 512u, initial + 5u * 512u, 4u * 512u) == 0 &&
        sb_vfs_directory_close(&dir) == SB_VFS_OBJECT_OK,
        "mkdir grows a full parent without overwriting existing entries")) return 1;
    if (finish() || start(1)) return 1;
    fail_barrier = 3u;
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_IO && !adapter.fs.write_faulted &&
        fat(durable, 0u, 6u) == 0u && fat(durable, 1u, 6u) == 0u &&
        fat(durable, 0u, 3u) >= 0x0FFFFFF8u, "failed parent growth releases child")) return 1;
    if (finish() || start(0)) return 1;
    const char *invalid[] = {"/disk/TOOLONG99", "/disk/A.LONG", "/disk/A B", "/disk/X/", "/disk/X/.", "/disk/X/..", "relative"};
    for (uint32_t i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        if (check(mkdir_path(invalid[i], &dir) != SB_VFS_OBJECT_OK && !dir.open, "invalid paths rejected")) return 1;
    if (check(mkdir_path("/disk", &dir) == SB_VFS_OBJECT_EXISTS &&
        mkdir_path("/disk/MISSING/X", &dir) == SB_VFS_OBJECT_NOT_FOUND &&
        mkdir_path("/disk/LOCKED/X", &dir) == SB_VFS_OBJECT_ACCESS &&
        sb_block_cache_stats().dirty_writes == 0u, "mount roots, parents and read-only attributes protected")) return 1;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 6u; c < 31u; ++c) put32(disk + (3u + copy) * 512u + c * 4u, 0x0FFFFFFFu);
    sb_block_cache_reset();
    if (check(mkdir_path("/disk/NOSPACE", &dir) == SB_VFS_OBJECT_RANGE &&
        sb_block_cache_stats().dirty_writes == 0u, "no space rejected before mutation")) return 1;
    if (finish() || start(0)) return 1;
    put32(disk + 4u * 512u + 24u, 0x0FFFFFFFu); sb_block_cache_reset();
    if (check(mkdir_path("/disk/BADMIRROR", &dir) == SB_VFS_OBJECT_INVALID &&
        mkdir_path("/disk/MIRROR", &dir) == SB_VFS_OBJECT_IO && sb_block_cache_stats().dirty_writes == 0u,
        "invalid name and inconsistent allocation copies rejected")) return 1;
    if (finish() || start(1)) return 1;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 7u; c < 31u; ++c) put32(disk + (3u + copy) * 512u + c * 4u, 0x0FFFFFFFu);
    sb_block_cache_reset();
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_RANGE &&
        sb_block_cache_stats().dirty_writes == 0u && barriers == 0u,
        "full parent requires both child and growth space before writes")) return 1;
    if (finish() || start(1)) return 1;
    sb_vfs_node_t *node;
    for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i) {
        char name[12] = "F000.TXT";
        name[2] = (char)('0' + i / 10u); name[3] = (char)('0' + i % 10u);
        const char *lookup = i == 0u ? "NEST" : i == 1u ? "LOCKED" : name;
        if (check(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), lookup, strlen(lookup), &node) == SB_VFS_OBJECT_OK &&
            sb_vfs_node_release(node) == SB_VFS_OBJECT_OK, "populate inode cache")) return 1;
    }
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_RANGE &&
        sb_block_cache_stats().dirty_writes == 0u && barriers == 0u, "inode capacity reserved before allocation")) return 1;
    if (finish() || start(0)) return 1;
    put32(disk + 4u * 512u + 12u, 0u); sb_block_cache_reset();
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_IO &&
        sb_block_cache_stats().dirty_writes == 0u, "inconsistent parent-chain copies rejected")) return 1;
    if (finish() || start(0)) return 1;
    device.write = 0;
    if (finish()) return 1;
    sb_block_cache_reset();
    if (attach()) return 1;
    if (check(mkdir_path("/disk/NEWDIR", &dir) == SB_VFS_OBJECT_ACCESS &&
        sb_block_cache_stats().dirty_writes == 0u, "read-only device refuses mkdir")) return 1;
    if (finish()) return 1;
    device.write = write_disk;
    puts("fat32 mkdir host test OK"); return 0;
}
