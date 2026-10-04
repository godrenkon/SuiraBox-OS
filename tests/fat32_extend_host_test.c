#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"

#define SECTORS 64u
static uint8_t disk[SECTORS * 512u], original[sizeof(disk)];
static uint8_t payload[1200u], buffer[1200u];
static int fail_read = -1, fail_write = -1;
static uint32_t flushes, fail_barrier, fail_barrier_again;
static int ordering_error;
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_node_t *node;
static sb_vfs_file_t file;

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (8u * i));
}
static uint32_t fat(uint32_t copy, uint32_t cluster) {
    return get32(disk + (3u + copy) * 512u + cluster * 4u) & 0x0FFFFFFFu;
}
static uint32_t data_position(uint32_t offset) {
    const uint32_t chain[] = {3u, 5u, 7u};
    return (5u + (chain[offset / 1024u] - 2u) * 2u) * 512u + offset % 1024u;
}
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 extension test failed: %s\n", why);
    return 1;
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t n, void *p) {
    if (!sb_block_range_valid(d, lba, n)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_read) { fail_read = -1; return SB_BLOCK_IO_ERROR; }
    memcpy(p, disk + lba * 512u, n * 512u);
    return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t n, const void *p) {
    if (!sb_block_range_valid(d, lba, n)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == fail_write) { fail_write = -1; return SB_BLOCK_IO_ERROR; }
    const uint8_t *bytes = p;
    /* Observe backend writes, not merely dirty-cache operations. */
    if ((lba == 3u || lba == 4u) &&
        (get32(bytes + 12u) & 0x0FFFFFFFu) == 5u) {
        if (flushes < 1u) ordering_error = 1;
        for (uint32_t i = 0u; i < sizeof(payload); ++i)
            if (disk[data_position(900u + i)] != payload[i]) ordering_error = 1;
    }
    if (lba == 5u && get32(bytes + 28u) == 2100u &&
        (flushes < 2u || fat(0u, 3u) != 5u || fat(1u, 3u) != 5u)) ordering_error = 1;
    memcpy(disk + lba * 512u, p, n * 512u);
    return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++flushes;
    if (flushes == fail_barrier || flushes == fail_barrier_again) return SB_BLOCK_IO_ERROR;
    return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "fat32-extension", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void dirent(uint32_t sector, uint32_t slot, const char *name, uint32_t cluster, uint32_t size, uint8_t attr) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attr; p[26] = (uint8_t)cluster;
    put32(p + 28u, size);
}
static int start(void) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 2u; disk[14] = 3u; disk[16] = 2u; disk[48] = 1u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    put32(disk + 512u, 0x41615252u); put32(disk + 512u + 484u, 0x61417272u);
    put32(disk + 512u + 508u, 0xAA550000u);
    put32(disk + 512u + 488u, 20u); put32(disk + 512u + 492u, 5u);
    const uint32_t occupied[] = {0u, 1u, 2u, 3u, 4u, 6u, 8u, 9u};
    for (uint32_t copy = 0u; copy < 2u; ++copy) {
        uint8_t *p = disk + (3u + copy) * 512u;
        for (uint32_t i = 0u; i < 128u; ++i) put32(p + i * 4u, copy ? 0xB0000000u : 0xA0000000u);
        for (uint32_t i = 0u; i < sizeof(occupied) / sizeof(occupied[0]); ++i)
            put32(p + occupied[i] * 4u, (copy ? 0xB0000000u : 0xA0000000u) | 0x0FFFFFFFu);
    }
    dirent(5u, 0u, "DATA    BIN", 3u, 900u, 0x20u);
    dirent(5u, 1u, "GUARD   BIN", 4u, 1024u, 0x20u);
    dirent(5u, 2u, "EMPTY   BIN", 0u, 0u, 0x20u);
    dirent(5u, 3u, "NEST       ", 8u, 0u, SB_FAT32_ATTR_DIRECTORY);
    dirent(17u, 0u, "DATA    BIN", 9u, 900u, 0x20u);
    memset(disk + 7u * 512u, 0x39, 1024u);
    memset(disk + 9u * 512u, 0x55, 1024u);
    memset(disk + 11u * 512u, 0xCC, 1024u);
    memset(disk + 15u * 512u, 0xDD, 1024u);
    memcpy(original, disk, sizeof(disk));
    sb_block_cache_reset(); flushes = 0u; ordering_error = 0;
    fail_read = fail_write = -1; fail_barrier = fail_barrier_again = 0u;
    return check(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
        sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "DATA.BIN", 8u, &node) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_open(node, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_seek(&file, 900u) == SB_VFS_OBJECT_OK, "fixture opens");
}
static int finish(void) {
    return check(sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_node_release(node) == SB_VFS_OBJECT_OK &&
        sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "references released");
}
static int unchanged_metadata(void) {
    return memcmp(disk + 3u * 512u, original + 3u * 512u, 3u * 512u) == 0;
}
int main(void) {
    uint64_t written;
    for (uint32_t i = 0u; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0xA5u);
    if (start()) return 1;
    sb_vfs_file_t reader;
    if (check(sb_vfs_file_open(node, SB_VFS_ACCESS_READ, &reader) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_OK &&
        written == sizeof(payload) && file.offset == 2100u && node->size == 2100u && flushes == 2u &&
        get32(disk + 5u * 512u + 28u) == 900u, "data and FAT durable before size publication")) return 1;
    sb_vfs_node_t *reopened;
    if (check(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "DATA.BIN", 8u, &reopened) == SB_VFS_OBJECT_OK &&
        reopened == node && sb_vfs_node_release(reopened) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_seek(&reader, 900u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&reader, buffer, sizeof(buffer), &written) == SB_VFS_OBJECT_OK &&
        written == sizeof(buffer) && memcmp(buffer, payload, sizeof(buffer)) == 0,
        "existing reader and reopen share the grown node")) return 1;
    fail_barrier = 3u;
    if (check(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && !ordering_error &&
        fat(0u, 3u) == 5u && fat(1u, 5u) == 7u && fat(0u, 7u) >= 0x0FFFFFF8u &&
        get32(disk + 5u * 512u + 28u) == 2100u &&
        get32(disk + 512u + 488u) == UINT32_MAX && get32(disk + 512u + 492u) == UINT32_MAX,
        "directory barrier retry and FSInfo invalidation")) return 1;
    if (check((get32(disk + 3u * 512u + 20u) & 0xF0000000u) == 0xA0000000u &&
        (get32(disk + 4u * 512u + 20u) & 0xF0000000u) == 0xB0000000u &&
        memcmp(disk + 9u * 512u, original + 9u * 512u, 1024u) == 0 &&
        memcmp(disk + 5u * 512u + 32u, original + 5u * 512u + 32u, 480u) == 0,
        "reserved nibbles, guard and neighboring directory entries preserved")) return 1;
    for (uint32_t i = 52u; i < 1024u; ++i)
        if (check(disk[15u * 512u + i] == 0u, "new cluster slack zeroed")) return 1;
    sb_block_cache_reset();
    if (check(sb_vfs_file_seek(&reader, 900u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&reader, buffer, sizeof(buffer), &written) == SB_VFS_OBJECT_OK &&
        memcmp(buffer, payload, sizeof(buffer)) == 0 &&
        sb_vfs_file_close(&reader) == SB_VFS_OBJECT_OK, "grown file survives cache reset")) return 1;
    if (finish()) return 1;
    /* Failure before FAT mutation and failure while flushing the second FAT. */
    for (uint32_t fault = 0u; fault < 4u; ++fault) {
        if (start()) return 1;
        if (fault == 0u) fail_barrier = 1u;
        if (fault == 1u) fail_write = 4;
        if (fault == 2u) fail_barrier = 2u;
        if (fault == 3u) fail_read = 5;
        if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_IO &&
            written == 0u && file.offset == 900u && node->size == 900u && !adapter.fs.write_faulted &&
            unchanged_metadata(), "failed extension restores old FAT and directory")) return 1;
        if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_OK &&
            sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK, "extension retries after rollback")) return 1;
        if (finish()) return 1;
    }
    if (start()) return 1;
    fail_barrier = 2u; fail_barrier_again = 3u;
    if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_IO &&
        adapter.fs.write_faulted && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO &&
        sb_vfs_file_write(&file, payload, 1u, &written) == SB_VFS_OBJECT_IO && written == 0u,
        "failed rollback quarantines all writes and sync")) return 1;
    if (finish()) return 1;
    if (start()) return 1;
    /* No space, inconsistent copies, overflow and request bound fail before mutation. */
    for (uint32_t copy = 0u; copy < 2u; ++copy)
        for (uint32_t c = 2u; c < 31u; ++c) put32(disk + (3u + copy) * 512u + c * 4u, 0x0FFFFFFFu);
    sb_block_cache_reset();
    if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_RANGE &&
        written == 0u && sb_block_cache_stats().dirty_writes == 0u, "full volume rejected before data write")) return 1;
    if (finish() || start()) return 1;
    if (check(sb_vfs_file_write(&file, payload, 9000u, &written) == SB_VFS_OBJECT_RANGE &&
        written == 0u && sb_block_cache_stats().dirty_writes == 0u,
        "eight-cluster allocation bound checked before source access")) return 1;
    put32(disk + 4u * 512u + 20u, 0x0FFFFFFFu); sb_block_cache_reset();
    if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_IO &&
        sb_block_cache_stats().dirty_writes == 0u, "inconsistent free-cluster mirrors rejected")) return 1;
    if (check(sb_fat32_write_file(&adapter.fs, &adapter.nodes[0].entry, 900u, UINT32_MAX,
        payload, &written) == SB_VFS_OBJECT_RANGE, "32-bit size overflow rejected")) return 1;
    if (finish() || start()) return 1;
    sb_vfs_node_t *empty;
    sb_vfs_file_t empty_file;
    if (check(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "EMPTY.BIN", 9u, &empty) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_open(empty, SB_VFS_ACCESS_ALL, &empty_file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&empty_file, payload, 100u, &written) == SB_VFS_OBJECT_OK &&
        empty->size == 100u && sb_vfs_file_sync(&empty_file) == SB_VFS_OBJECT_OK &&
        disk[5u * 512u + 64u + 26u] == 5u && get32(disk + 5u * 512u + 64u + 28u) == 100u &&
        sb_vfs_file_seek(&empty_file, 0u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&empty_file, buffer, 100u, &written) == SB_VFS_OBJECT_OK &&
        memcmp(buffer, payload, 100u) == 0, "empty file receives first cluster and persisted size")) return 1;
    if (check(sb_vfs_file_close(&empty_file) == SB_VFS_OBJECT_OK && sb_vfs_node_release(empty) == SB_VFS_OBJECT_OK,
        "empty file closes")) return 1;
    sb_vfs_node_t *dir, *nested;
    sb_vfs_file_t nested_file;
    if (check(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "NEST", 4u, &dir) == SB_VFS_OBJECT_OK &&
        sb_vfs_node_lookup(dir, "DATA.BIN", 8u, &nested) == SB_VFS_OBJECT_OK && nested != node &&
        sb_vfs_file_open(nested, SB_VFS_ACCESS_ALL, &nested_file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_seek(&nested_file, 900u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&nested_file, payload, 10u, &written) == SB_VFS_OBJECT_OK &&
        nested->size == 910u && node->size == 900u &&
        sb_vfs_file_sync(&nested_file) == SB_VFS_OBJECT_OK && get32(disk + 17u * 512u + 28u) == 910u &&
        sb_vfs_file_close(&nested_file) == SB_VFS_OBJECT_OK &&
        sb_vfs_node_release(nested) == SB_VFS_OBJECT_OK && sb_vfs_node_release(dir) == SB_VFS_OBJECT_OK,
        "nested growth updates its own directory slot without aliasing same-name file")) return 1;
    if (finish() || start()) return 1;
    if (check(sb_vfs_file_write(&file, payload, 100u, &written) == SB_VFS_OBJECT_OK &&
        node->size == 1000u && flushes == 1u &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && fat(0u, 3u) >= 0x0FFFFFF8u,
        "growth within existing cluster leaves FAT unchanged")) return 1;
    if (finish()) return 1;
    if (start()) return 1;
    if (check(sb_vfs_file_seek(&file, 880u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, payload, 200u, &written) == SB_VFS_OBJECT_OK &&
        written == 200u && node->size == 1080u && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_seek(&file, 880u) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&file, buffer, 200u, &written) == SB_VFS_OBJECT_OK &&
        memcmp(buffer, payload, 200u) == 0 && disk[7u * 512u + 879u] == 0x39u,
        "overwrite crossing EOF preserves prefix and extends correctly")) return 1;
    if (finish() || start()) return 1;
    disk[40] = 0x81u; put32(disk + 3u * 512u + 12u, 0u);
    sb_block_cache_reset();
    sb_fat32_t active;
    sb_fat32_dirent_t entry;
    if (check(sb_fat32_mount(&mount, &active) && active.active_fat == 1u && !active.mirrored &&
        sb_fat32_root_entry(&active, 0u, &entry) == SB_FAT32_DIRENT_OK &&
        sb_fat32_read_file(&active, &entry, 0u, 100u, buffer) && buffer[0] == 0x39u &&
        sb_fat32_write_file(&active, &entry, 900u, 1u, payload, &written) == SB_VFS_OBJECT_NOT_SUPPORTED &&
        written == 0u && sb_block_cache_stats().dirty_writes == 0u,
        "active FAT reads and unsupported nonmirrored extension")) return 1;
    if (finish()) return 1;
    puts("fat32 extension host test OK");
    return 0;
}
