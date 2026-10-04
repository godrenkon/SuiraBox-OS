#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 32u
static uint8_t disk[SECTORS * 512u], durable[sizeof(disk)], baseline[sizeof(disk)];
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_namespace_t ns;
static uint32_t barriers, reads, writes, clean_writes;
static int active, finalizing, ordering_error, fail_write = -1, fail_read = -1;
static uint32_t fail_barrier;
static int invalidate_status, corrupt_status;
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 unmount test failed: %s\n", why); return 1;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    ++reads;
    if ((int)lba == fail_read) return SB_BLOCK_IO_ERROR;
    memcpy(p, disk + lba * 512u, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, const void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    ++writes;
    if ((int)lba == fail_write) return SB_BLOCK_IO_ERROR;
    if (finalizing && (lba == 1u || lba == 2u) && (get32((const uint8_t *)p + 4u) & SB_FAT32_CLEAN_SHUTDOWN)) {
        ++clean_writes;
        if (durable[4u * 512u] != 'X' || barriers < 2u ||
            (!active && memcmp(durable + 3u * 512u + 32u, "NEW     TXT", 11u))) ordering_error = 1;
    }
    memcpy(disk + lba * 512u, p, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++barriers;
    if (barriers == fail_barrier) return SB_BLOCK_IO_ERROR;
    memcpy(durable, disk, sizeof(disk));
    /* Fault tests force cache misses only after data is durably flushed. */
    if (finalizing && invalidate_status) {
        if (corrupt_status) put32(disk + 512u + 4u, get32(disk + 512u + 4u) & ~SB_FAT32_NO_HARD_ERROR);
        sb_block_cache_invalidate(d, 1u, 2u);
        invalidate_status = 0;
    }
    return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "unmount", .sector_size = 512u, .sector_count = SECTORS,
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
    memcpy(durable, disk, sizeof(disk)); memcpy(baseline, disk, sizeof(disk));
    sb_block_cache_reset(); barriers = reads = writes = clean_writes = 0u;
    fail_write = fail_read = -1; fail_barrier = 0u;
    active = selected_active; finalizing = ordering_error = invalidate_status = corrupt_status = 0;
    return attach();
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "all references released");
}
static int mutate(void) {
    sb_vfs_file_t file;
    uint64_t n;
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, "X", 1u, &n) == SB_VFS_OBJECT_OK && n == 1u && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
        "unsynced overwrite accepted")) return 1;
    if (!active && check(sb_vfs_namespace_create_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "unsynced creation accepted")) return 1;
    finalizing = 1; return 0;
}
int main(void) {
    sb_vfs_file_t file;
    sb_vfs_directory_t directory;
    sb_vfs_node_t *node;
    uint64_t n;
    uint8_t data[8];
    for (int mode = 0; mode < 2; ++mode) {
        if (start(mode) || mutate()) return 1;
        if (check(sb_vfs_namespace_unmount_volume(&ns, "/disk/./", 8u) == SB_VFS_OBJECT_OK && ns.mount_count == 0u &&
            adapter.fs.quiesced && !adapter.fs.dirty_marked && !ordering_error && barriers == 3u &&
            clean_writes == (mode ? 1u : 2u) && sb_fat32_vfs_root(&adapter) == 0,
            "data barrier precedes clean-marker barrier and namespace detach")) return 1;
        for (uint32_t copy = 0u; copy < 2u; ++copy)
            if (check(get32(durable + (copy + 1u) * 512u + 4u) == get32(baseline + (copy + 1u) * 512u + 4u),
                "clean status preserves hard-error and reserved bits, including inactive FAT")) return 1;
        if (check(sb_fat32_begin_write(&adapter.fs) == SB_VFS_OBJECT_ACCESS &&
            sb_vfs_namespace_mount(&ns, "/stale", 6u, &adapter.root) == SB_VFS_OBJECT_INVALID,
            "finalized adapter cannot mutate or remount via a borrowed root")) return 1;
        if (finish()) return 1;
        sb_block_cache_reset(); memcpy(disk, durable, sizeof(disk));
        if (attach()) return 1;
        if (check(!adapter.fs.recovery_flags &&
            sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_read(&file, data, sizeof(data), &n) == SB_VFS_OBJECT_OK && n == 5u && data[0] == 'X' &&
            sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "fresh mount is writable and retains unsynced payload")) return 1;
        if (!mode && check(sb_vfs_namespace_open_file(&ns, "/disk/NEW.TXT", 13u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "unsynced directory entry survived finalization")) return 1;
        if (finish()) return 1;
    }
    if (start(0) || mutate()) return 1;
    const uint32_t before = barriers, before_writes = writes;
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_BUSY &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_open_directory(&ns, "/disk", 5u, &directory) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_BUSY &&
        sb_vfs_directory_close(&directory) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_resolve(&ns, "/disk", 5u, &node) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_BUSY &&
        sb_vfs_node_release(node) == SB_VFS_OBJECT_OK && barriers == before && writes == before_writes && !adapter.fs.quiesced,
        "file, directory, and borrowed root references refuse unmount before I/O")) return 1;
    sb_vfs_namespace_t alias;
    sb_vfs_namespace_init(&alias);
    if (check(sb_vfs_namespace_mount(&alias, "/alias", 6u, &adapter.root) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_BUSY && !adapter.fs.quiesced,
        "another namespace alias prevents finalization")) return 1;
    sb_vfs_namespace_destroy(&alias);
    if (check(sb_vfs_namespace_mount(&ns, "/disk/nested", 12u, &adapter.root) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_BUSY && !adapter.fs.quiesced,
        "nested mount prevents parent finalization")) return 1;
    if (finish()) return 1;
    for (uint32_t fault = 0u; fault < 8u; ++fault) {
        if (start(0) || mutate()) return 1;
        if (fault == 0u) fail_write = 4;
        if (fault == 1u) fail_barrier = 2u;
        if (fault == 2u || fault == 3u) fail_write = (int)fault - 1;
        if (fault == 4u || fault == 5u) { fail_read = (int)fault - 3; invalidate_status = 1; }
        if (fault == 6u) fail_barrier = 3u;
        if (fault == 7u) { invalidate_status = 1; corrupt_status = 1; }
        if (check(sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_IO && ns.mount_count == 1u &&
            adapter.fs.quiesced && adapter.fs.write_faulted && adapter.fs.dirty_marked && !ordering_error,
            "data/status/barrier failure keeps namespace readable and seals mutation")) return 1;
        if (fault != 6u && check(clean_writes == 0u || fault == 3u,
            "no clean marker accepted before completed data durability and validated statuses")) return 1;
        fail_write = fail_read = -1; fail_barrier = 0u;
        if (check(sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_read(&file, data, sizeof(data), &n) == SB_VFS_OBJECT_OK && data[0] == 'X' &&
            sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK && sb_fat32_begin_write(&adapter.fs) == SB_VFS_OBJECT_IO &&
            sb_vfs_namespace_open_file(&ns, "/disk/RUNTIME.TXT", 17u, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
            sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_IO,
            "sealed failure allows reads but cannot resume writes or mark clean on retry")) return 1;
        if (finish()) return 1;
    }
    for (uint32_t reason = 0u; reason < 4u; ++reason) {
        if (start(0) || finish()) return 1;
        if (reason == 0u) device.write = 0;
        if (reason == 1u) for (uint32_t c = 0u; c < 2u; ++c)
            put32(disk + (c + 1u) * 512u + 4u, get32(disk + (c + 1u) * 512u + 4u) & ~SB_FAT32_CLEAN_SHUTDOWN);
        if (reason == 2u) for (uint32_t c = 0u; c < 2u; ++c)
            put32(disk + (c + 1u) * 512u + 4u, get32(disk + (c + 1u) * 512u + 4u) & ~SB_FAT32_NO_HARD_ERROR);
        if (reason == 3u) put32(disk + 512u + 4u, get32(disk + 512u + 4u) & ~SB_FAT32_CLEAN_SHUTDOWN);
        memcpy(baseline, disk, sizeof(disk)); sb_block_cache_reset();
        if (attach()) return 1;
        const uint32_t saved_reads = reads;
        if (check(sb_vfs_namespace_unmount_volume(&ns, "/disk", 5u) == SB_VFS_OBJECT_OK &&
            reads == saved_reads && barriers == 0u && writes == 0u && memcmp(disk, baseline, sizeof(disk)) == 0,
            "read-only and recovery unmount preserve evidence without disk I/O")) return 1;
        if (finish()) return 1;
        device.write = write_disk;
    }
    puts("fat32 unmount host test OK"); return 0;
}
