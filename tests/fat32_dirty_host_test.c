#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 32u
static uint8_t disk[SECTORS * 512u], durable[sizeof(disk)], initial[sizeof(disk)];
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_namespace_t ns;
static uint32_t barriers, data_writes;
static int fail_read = -1, fail_write = -1, fail_barrier, ordering_error, active;
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 dirty test failed: %s\n", why); return 1;
}
static int dirty_durable(void) {
    for (uint32_t copy = active ? 1u : 0u; copy < 2u; ++copy)
        if ((get32(durable + (copy + 1u) * 512u + 4u) & SB_FAT32_CLEAN_SHUTDOWN) != 0u) return 0;
    return barriers != 0u;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_read) return SB_BLOCK_IO_ERROR;
    memcpy(p, disk + lba * 512u, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, const void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_write) return SB_BLOCK_IO_ERROR;
    if (lba >= 3u) {
        ++data_writes;
        if (!dirty_durable()) ordering_error = 1;
    }
    memcpy(disk + lba * 512u, p, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++barriers;
    if (fail_barrier) return SB_BLOCK_IO_ERROR;
    memcpy(durable, disk, sizeof(disk)); return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "dirty", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static int attach(void) {
    sb_vfs_namespace_init(&ns);
    return check(sb_vfs_mount(&device, &mount) == SB_VFS_OK && sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK, "fixture mounted");
}
static int start(int selected_active) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 1u; disk[14] = 1u; disk[16] = 2u;
    if (selected_active) disk[40] = 0x81u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 0u; c < 4u; ++c)
            put32(disk + (copy + 1u) * 512u + c * 4u, (copy ? 0xB0000000u : 0xA0000000u) | 0x0FFFFFFFu);
    memcpy(disk + 3u * 512u, "RUNTIME TXT", 11u);
    disk[3u * 512u + 11u] = 0x20u; disk[3u * 512u + 26u] = 3u; put32(disk + 3u * 512u + 28u, 5u);
    memcpy(disk + 4u * 512u, "hello", 5u);
    memcpy(initial, disk, sizeof(disk)); memcpy(durable, disk, sizeof(disk));
    sb_block_cache_reset(); barriers = data_writes = 0u;
    fail_read = fail_write = -1; fail_barrier = ordering_error = 0; active = selected_active;
    if (attach()) return 1;
    sb_block_cache_reset(); return 0;
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "references released");
}
int main(void) {
    sb_vfs_file_t file;
    sb_vfs_directory_t directory;
    uint64_t n;
    uint8_t buffer[8];
    for (uint32_t mutation = 0u; mutation < 5u; ++mutation) {
        if (start(mutation == 4u)) return 1;
        if (mutation < 2u || mutation == 4u) {
            if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_seek(&file, mutation == 1u ? 5u : 0u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_write(&file, "X", 1u, &n) == SB_VFS_OBJECT_OK && n == 1u &&
                sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
                "overwrite and extension mark dirty before data writeback")) return 1;
        } else if (mutation == 2u) {
            if (check(sb_vfs_namespace_create_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
                barriers == 1u && data_writes == 0u && memcmp(durable + 3u * 512u, initial + 3u * 512u, 512u) == 0 &&
                sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK && barriers == 1u,
                "unsynced create/close keeps publication cached while marker is durable")) return 1;
        } else {
            if (check(sb_vfs_namespace_create_directory(&ns, "/disk/NEW", 9u, &directory) == SB_VFS_OBJECT_OK &&
                sb_vfs_directory_close(&directory) == SB_VFS_OBJECT_OK,
                "directory initialization and publication follow dirty barrier")) return 1;
        }
        if (check(adapter.fs.dirty_marked && !adapter.fs.recovery_flags && dirty_durable() && !ordering_error &&
            (get32(durable + 512u + 4u) & 0xF0000000u) == 0xA0000000u &&
            (get32(durable + 1024u + 4u) & 0xF0000000u) == 0xB0000000u &&
            (get32(durable + 1024u + 4u) & SB_FAT32_NO_HARD_ERROR) != 0u,
            "dirty session stays writable and preserves reserved and hard-error bits")) return 1;
        const uint32_t previous = barriers;
        if (check(sb_fat32_begin_write(&adapter.fs) == SB_VFS_OBJECT_OK && barriers == previous,
            "one dirty barrier per mount session")) return 1;
        if (mutation == 4u && check(get32(durable + 512u + 4u) == get32(initial + 512u + 4u),
            "active-FAT mode does not alter inactive status")) return 1;
        if (finish()) return 1;
        sb_block_cache_reset(); memcpy(disk, durable, sizeof(disk));
        if (attach()) return 1;
        if (check(adapter.fs.recovery_flags == SB_FAT32_RECOVERY_UNCLEAN &&
            sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
            sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_read(&file, buffer, sizeof(buffer), &n) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
            "new mount sees persistent dirty evidence and permits only reads")) return 1;
        if (mutation == 2u && check(sb_vfs_namespace_open_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_NOT_FOUND,
            "cache-loss crash discards unsynced creation but retains dirty marker")) return 1;
        if (finish()) return 1;
    }
    for (uint32_t fault = 0u; fault < 5u; ++fault) {
        if (start(0)) return 1;
        if (fault < 2u) fail_read = (int)fault + 1;
        if (fault == 2u || fault == 3u) fail_write = (int)fault - 1;
        if (fault == 4u) fail_barrier = 1;
        const int result = fault < 2u ? sb_fat32_begin_write(&adapter.fs)
            : sb_vfs_namespace_create_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_ALL, &file);
        if (check(result == SB_VFS_OBJECT_IO && adapter.fs.write_faulted && !adapter.fs.dirty_marked &&
            data_writes == 0u && memcmp(disk + 3u * 512u, initial + 3u * 512u, sizeof(disk) - 3u * 512u) == 0,
            "marker read/write/barrier failure quarantines without touching data")) return 1;
        fail_read = fail_write = -1; fail_barrier = 0;
        if (check(sb_vfs_namespace_create_file(&ns, "/disk/OTHER.TXT", 15u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_IO && !file.open,
            "uncertain marker cannot retry mutation on this mount")) return 1;
        for (uint32_t i = 0u; i < SB_FAT32_VFS_NODE_CACHE; ++i)
            if (check(!adapter.nodes[i].in_use, "marker failure releases reserved inode slot")) return 1;
        if (finish()) return 1;
    }
    if (start(0)) return 1;
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, "", 0u, &n) == SB_VFS_OBJECT_OK && n == 0u &&
        sb_vfs_file_seek(&file, 6u) == SB_VFS_OBJECT_RANGE && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_create_file(&ns, "/disk/TOOLONG99.TXT", 19u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_INVALID &&
        !adapter.fs.dirty_marked && barriers == 0u && data_writes == 0u && memcmp(disk, initial, sizeof(disk)) == 0,
        "zero-length and rejected requests do not mark volume dirty")) return 1;
    if (finish()) return 1;
    puts("fat32 dirty host test OK"); return 0;
}
