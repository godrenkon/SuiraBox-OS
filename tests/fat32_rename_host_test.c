#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 64u
static uint8_t disk[SECTORS * 512u], durable[sizeof(disk)], seed[sizeof(disk)];
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_namespace_t ns;
static uint32_t barriers, writes;
static int active, ordering_error, staged, fail_read = -1, fail_write = -1;
static uint32_t fail_barrier;
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 rename failed: %s\n", why); return 1;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t n, void *p) {
    if (!sb_block_range_valid(d, lba, n)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_read) return SB_BLOCK_IO_ERROR;
    memcpy(p, disk + lba * 512u, n * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t n, const void *p) {
    if (!sb_block_range_valid(d, lba, n)) return SB_BLOCK_INVALID_ARGUMENT;
    ++writes;
    if ((int)lba == fail_write) return SB_BLOCK_IO_ERROR;
    if (lba >= 3u) {
        for (uint32_t copy = active ? 1u : 0u; copy < 2u; ++copy)
            if (get32(durable + (copy + 1u) * 512u + 4u) & SB_FAT32_CLEAN_SHUTDOWN) ordering_error = 1;
        if (lba == 3u && !memcmp(p, "PUBLISH DAT", 11u) &&
            (barriers < 2u || (staged && durable[4u * 512u] != 'X'))) ordering_error = 1;
    }
    memcpy(disk + lba * 512u, p, n * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    if (++barriers == fail_barrier) return SB_BLOCK_IO_ERROR;
    memcpy(durable, disk, sizeof(disk)); return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "rename", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void entry(uint8_t *p, const char *name, uint8_t attr, uint32_t cluster, uint32_t size) {
    memset(p, 0, 32u); memcpy(p, name, 11u); p[11] = attr;
    p[26] = (uint8_t)cluster; put32(p + 28u, size);
}
static int attach(void) {
    sb_vfs_namespace_init(&ns);
    return check(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
        sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK, "mount");
}
static int start(int mode) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 1u; disk[14] = 1u; disk[16] = 2u;
    if (mode) disk[40] = 0x81u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 0u; c <= 8u; ++c)
            put32(disk + (copy + 1u) * 512u + c * 4u, (copy ? 0xB0000000u : 0xA0000000u) | 0x0FFFFFFFu);
    entry(disk + 3u * 512u, "SOURCE  TMP", 0x20u, 3u, 5u);
    /* Non-name bytes must survive publication exactly. */
    disk[3u * 512u + 12u] = 0x18u; disk[3u * 512u + 13u] = 7u; disk[3u * 512u + 14u] = 0x31u;
    entry(disk + 3u * 512u + 32u, "OTHER   TXT", 0x20u, 0u, 0u);
    entry(disk + 3u * 512u + 64u, "SAVES      ", 0x10u, 4u, 0u);
    entry(disk + 3u * 512u + 96u, "LOCKED     ", 0x11u, 5u, 0u);
    disk[3u * 512u + 128u] = 0x41u; disk[3u * 512u + 139u] = 0x0Fu;
    entry(disk + 3u * 512u + 160u, "LFNALIAS TMP", 0x20u, 6u, 1u);
    entry(disk + 3u * 512u + 192u, "READONLYTMP", 0x21u, 0u, 0u);
    memcpy(disk + 4u * 512u, "hello", 5u);
    for (uint32_t sector = 5u; sector <= 6u; ++sector) {
        entry(disk + sector * 512u, ".          ", 0x10u, sector - 1u, 0u);
        entry(disk + sector * 512u + 32u, "..         ", 0x10u, 0u, 0u);
        entry(disk + sector * 512u + 64u, "SOURCE  TMP", 0x20u, sector + 2u, 5u);
    }
    memcpy(disk + 7u * 512u, "L", 1u); memcpy(disk + 8u * 512u, "world", 5u);
    memcpy(disk + 9u * 512u, "guard", 5u);
    /* Inactive FAT disagreement must not reject a selected active FAT. */
    if (mode) put32(disk + 512u + 8u, 0xA0000000u);
    memcpy(seed, disk, sizeof(disk)); memcpy(durable, disk, sizeof(disk));
    sb_block_cache_reset(); barriers = writes = fail_barrier = 0u;
    fail_read = fail_write = -1; active = mode; ordering_error = staged = 0;
    return attach();
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "references released");
}
static int rename_file(const char *a, const char *b) {
    return sb_vfs_namespace_rename_file(&ns, a, strlen(a), b, strlen(b));
}
int main(void) {
    sb_vfs_file_t file, alias;
    uint64_t n;
    char data[8];
    for (int mode = 0; mode < 2; ++mode) {
        if (start(mode)) return 1;
        if (check(sb_vfs_namespace_open_file(&ns, "/disk/SOURCE.TMP", 16u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_seek(&file, 1u) == SB_VFS_OBJECT_OK &&
            rename_file("/disk/./source.tmp", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_OK &&
            barriers == 3u && !ordering_error, "ordered exclusive rename")) return 1;
        if (check(sb_vfs_namespace_open_file(&ns, "/disk/SOURCE.TMP", 16u, SB_VFS_ACCESS_READ, &alias) == SB_VFS_OBJECT_NOT_FOUND &&
            sb_vfs_namespace_open_file(&ns, "/disk/PUBLISH.DAT", 17u, SB_VFS_ACCESS_ALL, &alias) == SB_VFS_OBJECT_OK &&
            file.node == alias.node && file.offset == 1u &&
            sb_vfs_file_read(&file, data, 4u, &n) == SB_VFS_OBJECT_OK && n == 4u && !memcmp(data, "ello", 4u) &&
            sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&alias) == SB_VFS_OBJECT_OK,
            "name changed but held file identity and offset preserved")) return 1;
        uint8_t expected[sizeof(disk)]; memcpy(expected, seed, sizeof(seed));
        memcpy(expected + 3u * 512u, "PUBLISH DAT", 11u);
        for (uint32_t copy = mode ? 1u : 0u; copy < 2u; ++copy)
            put32(expected + (copy + 1u) * 512u + 4u, get32(expected + (copy + 1u) * 512u + 4u) & ~SB_FAT32_CLEAN_SHUTDOWN);
        if (check(!memcmp(durable, expected, sizeof(expected)), "only SFN and required dirty status changed")) return 1;
        if (check(sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_OK, "clean unmount") || finish()) return 1;
        sb_block_cache_reset(); memcpy(disk, durable, sizeof(disk));
        if (attach()) return 1;
        if (check(!adapter.fs.recovery_flags && sb_vfs_namespace_open_file(&ns, "/disk/PUBLISH.DAT", 17u,
            SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK && sb_vfs_file_read(&file, data, 5u, &n) == SB_VFS_OBJECT_OK &&
            n == 5u && !memcmp(data, "hello", 5u) && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "cache-loss writable remount")) return 1;
        if (finish()) return 1;
    }
    if (start(0)) return 1;
    if (check(rename_file("/disk/source.tmp", "/disk/SOURCE.TMP") == SB_VFS_OBJECT_OK, "case-folded no-op")) return 1;
    const struct { const char *old, *new_name; int result; } rejected[] = {
        {"/disk/SOURCE.TMP", "/disk/OTHER.TXT", SB_VFS_OBJECT_EXISTS},
        {"/disk/MISSING.TMP", "/disk/PUBLISH.DAT", SB_VFS_OBJECT_NOT_FOUND},
        {"/disk/SAVES", "/disk/MOVED", SB_VFS_OBJECT_NOT_SUPPORTED},
        {"/disk/LFNALIAS.TMP", "/disk/PUBLISH.DAT", SB_VFS_OBJECT_NOT_SUPPORTED},
        {"/disk/SOURCE.TMP", "/disk/SAVES/WORLD.DAT", SB_VFS_OBJECT_NOT_SUPPORTED},
        {"/disk/LOCKED/SOURCE.TMP", "/disk/LOCKED/WORLD.DAT", SB_VFS_OBJECT_ACCESS},
        {"/disk/READONLY.TMP", "/disk/PUBLISH.DAT", SB_VFS_OBJECT_ACCESS},
        {"/disk/SOURCE.TMP", "/disk", SB_VFS_OBJECT_ACCESS},
        {"/disk/SOURCE.TMP", "/disk/TOOLONG99.DAT", SB_VFS_OBJECT_INVALID},
        {"/disk/SOURCE.TMP/", "/disk/PUBLISH.DAT", SB_VFS_OBJECT_INVALID},
        {"/disk/SOURCE.TMP", "/disk/.", SB_VFS_OBJECT_INVALID},
    };
    for (uint32_t i = 0u; i < sizeof(rejected) / sizeof(rejected[0]); ++i)
        if (check(rename_file(rejected[i].old, rejected[i].new_name) == rejected[i].result, rejected[i].old)) return 1;
    if (check(!writes && !barriers && !memcmp(disk, seed, sizeof(disk)), "rejections and no-op have no mutation")) return 1;
    if (check(rename_file("/disk/SAVES/SOURCE.TMP", "/disk/SAVES/WORLD.DAT") == SB_VFS_OBJECT_OK &&
        !memcmp(durable + 5u * 512u + 64u, "WORLD   DAT", 11u), "nested parent rename")) return 1;
    if (finish() || start(0)) return 1;
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/SOURCE.TMP", 16u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, "X", 1u, &n) == SB_VFS_OBJECT_OK && durable[4u * 512u] == 'h', "unsynced staged data")) return 1;
    staged = 1;
    if (check(rename_file("/disk/SOURCE.TMP", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_OK && !ordering_error &&
        durable[4u * 512u] == 'X' && sb_vfs_file_seek(&file, 5u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, "!", 1u, &n) == SB_VFS_OBJECT_OK && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
        !memcmp(durable + 3u * 512u, "PUBLISH DAT", 11u) && get32(durable + 3u * 512u + 28u) == 6u &&
        !memcmp(durable + 4u * 512u, "Xello!", 6u) && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
        "staged bytes durable before publication; held handle extends new name")) return 1;
    if (finish()) return 1;
    for (int fault = 0; fault < 7; ++fault) {
        if (start(0)) return 1;
        if (fault == 0) fail_read = 3;
        if (fault == 1) fail_write = 1;
        if (fault == 2) fail_write = 2;
        if (fault >= 3 && fault <= 5) fail_barrier = (uint32_t)fault - 2u;
        if (fault == 6) fail_write = 3;
        if (check(rename_file("/disk/SOURCE.TMP", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_IO && !ordering_error &&
            adapter.fs.write_faulted == (fault != 0), "read/marker/prepublish/publish/barrier failure contract")) return 1;
        if (fault == 0) {
            fail_read = -1;
            if (check(!writes && !barriers && rename_file("/disk/SOURCE.TMP", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_OK,
                "read-only preflight failure is retryable")) return 1;
        } else if (check(rename_file("/disk/SOURCE.TMP", "/disk/AGAIN.DAT") == SB_VFS_OBJECT_IO &&
            sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_IO,
            "uncertain mutation forbids retry and clean status")) return 1;
        if (finish()) return 1;
    }
    if (start(0) || finish()) return 1;
    /* LFN prefix crosses a fragmented directory chain boundary. */
    memset(disk + 3u * 512u, 0, 512u);
    for (uint32_t i = 0u; i < 15u; ++i) {
        char name[12] = "FILL0000TMP"; name[7] = (char)('A' + i);
        entry(disk + 3u * 512u + i * 32u, name, 0x20u, 0u, 0u);
    }
    disk[3u * 512u + 480u] = 0x41u; disk[3u * 512u + 491u] = 0x0Fu;
    entry(disk + 10u * 512u, "SOURCE  TMP", 0x20u, 3u, 5u);
    for (uint32_t copy = 0u; copy < 2u; ++copy) {
        put32(disk + (copy + 1u) * 512u + 8u, 9u);
        put32(disk + (copy + 1u) * 512u + 36u, 0x0FFFFFFFu);
    }
    sb_block_cache_reset(); barriers = writes = 0u;
    if (attach()) return 1;
    if (check(rename_file("/disk/SOURCE.TMP", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_NOT_SUPPORTED && !writes && !barriers,
        "LFN prefix at previous fragment is protected")) return 1;
    if (finish() || start(0) || finish()) return 1;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        put32(disk + (copy + 1u) * 512u + 4u, get32(disk + (copy + 1u) * 512u + 4u) & ~SB_FAT32_CLEAN_SHUTDOWN);
    memcpy(seed, disk, sizeof(disk)); sb_block_cache_reset(); writes = barriers = 0u;
    if (attach()) return 1;
    if (check(rename_file("/disk/SOURCE.TMP", "/disk/PUBLISH.DAT") == SB_VFS_OBJECT_ACCESS &&
        adapter.root.ops->rename(&adapter.root, "SOURCE.TMP", 10u, "PUBLISH.DAT", 11u) == SB_VFS_OBJECT_ACCESS &&
        !writes && !barriers && !memcmp(seed, disk, sizeof(disk)), "recovery mount rejects direct and generic rename")) return 1;
    if (finish()) return 1;
    puts("FAT32 rename host test OK"); return 0;
}
