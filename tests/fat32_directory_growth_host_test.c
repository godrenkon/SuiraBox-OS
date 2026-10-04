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
static uint64_t new_lba;
static uint32_t grown_tail;

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 directory growth test failed: %s\n", why); return 1;
}
static uint32_t fat(const uint8_t *bytes, uint32_t copy, uint32_t cluster) {
    return get32(bytes + (3u + copy) * 512u + cluster * 4u) & 0x0FFFFFFFu;
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
    if ((lba == 3u || lba == 4u) &&
        (get32(bytes + grown_tail * 4u) & 0x0FFFFFFFu) == 6u) {
        if (barriers < 1u || memcmp(durable + new_lba * 512u, "GROW    TXT", 11u) != 0)
            ordering_error = 1;
        for (uint32_t i = 32u; i < 1024u; ++i)
            if (durable[new_lba * 512u + i] != 0u) ordering_error = 1;
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
    .name = "directory-growth", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void dirent(uint32_t sector, uint32_t slot, const char *name,
                    uint8_t attr, uint32_t cluster, uint32_t size) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attr; p[26] = (uint8_t)cluster; put32(p + 28u, size);
}
static void fill(uint32_t sector, uint32_t count, char prefix) {
    for (uint32_t i = 0u; i < count; ++i) {
        char name[12] = "F000    TXT";
        name[0] = prefix; name[1] = (char)('0' + i / 100u % 10u);
        name[2] = (char)('0' + i / 10u % 10u); name[3] = (char)('0' + i % 10u);
        dirent(sector + i / 16u, i % 16u, name, 0x20u, 0u, 0u);
    }
}
static int start(void) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 2u; disk[14] = 3u; disk[16] = 2u; disk[48] = 1u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    put32(disk + 512u, 0x41615252u); put32(disk + 512u + 484u, 0x61417272u);
    put32(disk + 512u + 508u, 0xAA550000u);
    put32(disk + 512u + 488u, 20u); put32(disk + 512u + 492u, 6u);
    for (uint32_t copy = 0u; copy < 2u; ++copy) {
        for (uint32_t c = 0u; c < 128u; ++c)
            put32(disk + (3u + copy) * 512u + c * 4u, (copy ? 0xB0000000u : 0xA0000000u) |
                (c < 6u ? (c == 2u ? 3u : 0x0FFFFFFFu) : 0u));
    }
    fill(5u, 64u, 'F'); /* Root chain 2 -> 3, two sectors in each cluster. */
    dirent(5u, 0u, "NEST       ", SB_FAT32_ATTR_DIRECTORY, 4u, 0u);
    dirent(8u, 15u, "GUARD   BIN", 0x20u, 5u, 256u);
    fill(9u, 32u, 'D');
    memset(disk + 11u * 512u, 0x55, 1024u);
    memset(disk + 13u * 512u, 0xCC, 1024u);
    memcpy(initial, disk, sizeof(disk)); memcpy(durable, disk, sizeof(disk));
    sb_block_cache_reset(); barriers = fail_barrier = fail_again = 0u;
    fail_read = fail_write = -1; ordering_error = 0; new_lba = 13u; grown_tail = 3u;
    sb_vfs_namespace_init(&ns);
    if (check(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
        sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK,
        "fixture mounted")) return 1;
    /* Keep data/FAT failure indices scoped after the dirty-status barrier. */
    if (check(sb_fat32_begin_write(&adapter.fs) == SB_VFS_OBJECT_OK, "dirty session prepared")) return 1;
    memcpy(initial, disk, sizeof(disk)); sb_block_cache_reset(); barriers = 0u;
    return 0;
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "references released");
}
static int create(const char *path, sb_vfs_file_t *file) {
    return sb_vfs_namespace_create_file(&ns, path, strlen(path), SB_VFS_ACCESS_ALL, file);
}
static int old_metadata(void) {
    return memcmp(durable + 3u * 512u, initial + 3u * 512u, 10u * 512u) == 0;
}
int main(void) {
    sb_vfs_file_t file;
    sb_vfs_node_t *node;
    if (start()) return 1;
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_OK && file.node->size == 0u &&
        barriers == 2u && !ordering_error && fat(durable, 0u, 3u) == 6u && fat(durable, 1u, 6u) >= 0x0FFFFFF8u &&
        memcmp(durable + 5u * 512u, initial + 5u * 512u, 8u * 512u) == 0 &&
        get32(durable + 512u + 488u) == UINT32_MAX && get32(durable + 512u + 492u) == UINT32_MAX,
        "new cluster and entry durable before mirrored FAT publication")) return 1;
    if (check((get32(durable + 3u * 512u + 12u) & 0xF0000000u) == 0xA0000000u &&
        (get32(durable + 4u * 512u + 24u) & 0xF0000000u) == 0xB0000000u &&
        sb_vfs_namespace_resolve(&ns, "/disk/GROW.TXT", 14u, &node) == SB_VFS_OBJECT_OK &&
        node == file.node && sb_vfs_node_release(node) == SB_VFS_OBJECT_OK,
        "reserved nibbles and cached node identity preserved")) return 1;
    fail_barrier = 3u;
    if (check(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "sync remains retryable after successful growth")) return 1;
    sb_block_cache_reset(); memcpy(disk, durable, sizeof(disk));
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/GROW.TXT", 14u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "grown directory survives cache discard and device restart")) return 1;
    if (finish()) return 1;
    /* Data failure, data barrier failure, FAT write/barrier failure and FSInfo read failure. */
    for (uint32_t fault = 0u; fault < 5u; ++fault) {
        if (start()) return 1;
        if (fault == 0u) fail_write = 13;
        if (fault == 1u) fail_barrier = 1u;
        if (fault == 2u) fail_write = 4;
        if (fault == 3u) fail_barrier = 2u;
        if (fault == 4u) fail_read = 1;
        const int result = create("/disk/GROW.TXT", &file);
        if (check(result == SB_VFS_OBJECT_IO && !file.open && !adapter.fs.write_faulted && old_metadata() &&
            sb_vfs_namespace_resolve(&ns, "/disk/GROW.TXT", 14u, &node) == SB_VFS_OBJECT_NOT_FOUND,
            "failed growth leaves old directory and allocation reachable state unchanged")) return 1;
        if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_OK && !ordering_error &&
            sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "failed growth releases node slot and retries")) return 1;
        if (finish()) return 1;
    }
    if (start()) return 1;
    fail_barrier = 2u; fail_again = 3u;
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_IO && !file.open && adapter.fs.write_faulted &&
        create("/disk/OTHER.TXT", &file) == SB_VFS_OBJECT_IO,
        "failed FAT rollback quarantines creation")) return 1;
    if (finish() || start()) return 1;
    grown_tail = 4u;
    if (check(create("/disk/NEST/GROW.TXT", &file) == SB_VFS_OBJECT_OK && !ordering_error &&
        fat(durable, 0u, 4u) == 6u && fat(durable, 1u, 3u) >= 0x0FFFFFF8u &&
        memcmp(durable + 9u * 512u, initial + 9u * 512u, 2u * 512u) == 0 &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "nested directory grows without changing root chain")) return 1;
    if (finish() || start()) return 1;
    put32(disk + 4u * 512u + 12u, 0u); sb_block_cache_reset();
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_IO &&
        sb_block_cache_stats().dirty_writes == 0u, "inconsistent directory FAT copies rejected before mutation")) return 1;
    if (finish() || start()) return 1;
    put32(disk + 4u * 512u + 24u, 0x0FFFFFFFu); sb_block_cache_reset();
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_IO &&
        sb_block_cache_stats().dirty_writes == 0u, "inconsistent free-cluster copies rejected")) return 1;
    if (finish() || start()) return 1;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 6u; c < 31u; ++c) put32(disk + (3u + copy) * 512u + c * 4u, 0x0FFFFFFFu);
    sb_block_cache_reset();
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_RANGE && !file.open &&
        sb_block_cache_stats().dirty_writes == 0u, "out-of-space rejection precedes writes")) return 1;
    if (finish() || start()) return 1;
    disk[8u * 512u + 480u + 11u] = SB_FAT32_ATTR_LONG_NAME; sb_block_cache_reset();
    if (check(create("/disk/GROW.TXT", &file) == SB_VFS_OBJECT_RANGE &&
        sb_block_cache_stats().dirty_writes == 0u, "orphan LFN at tail cannot attach across a new cluster")) return 1;
    if (finish()) return 1;
    puts("fat32 directory growth host test OK");
    return 0;
}
