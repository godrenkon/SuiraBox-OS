#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"

#define SECTORS 32u
#define FILE_BYTES 2500u
static uint8_t disk[SECTORS * SB_BLOCK_SECTOR_SIZE];
static uint8_t expected[sizeof(disk)];
static int fail_read_lba = -1;
static int fail_write;
static int fail_flush;
static uint32_t flushes;

static int require(int condition, const char *message) {
    if (condition) return 0;
    fprintf(stderr, "FAT32 write test failed: %s\n", message);
    return 1;
}

static void put32(uint8_t *p, uint32_t value) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(value >> (i * 8u));
}

static sb_block_status_t read_disk(sb_block_device_t *device, uint64_t lba,
                                   uint32_t count, void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || buffer == 0)
        return SB_BLOCK_INVALID_ARGUMENT;
    if (fail_read_lba >= 0 && lba == (uint64_t)fail_read_lba) {
        fail_read_lba = -1;
        return SB_BLOCK_IO_ERROR;
    }
    memcpy(buffer, disk + lba * SB_BLOCK_SECTOR_SIZE, count * SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t write_disk(sb_block_device_t *device, uint64_t lba,
                                    uint32_t count, const void *buffer) {
    if (!sb_block_range_valid(device, lba, count) || buffer == 0)
        return SB_BLOCK_INVALID_ARGUMENT;
    if (fail_write) { fail_write = 0; return SB_BLOCK_IO_ERROR; }
    memcpy(disk + lba * SB_BLOCK_SECTOR_SIZE, buffer, count * SB_BLOCK_SECTOR_SIZE);
    return SB_BLOCK_OK;
}

static sb_block_status_t flush_disk(sb_block_device_t *device) {
    if (device == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++flushes;
    if (fail_flush) { fail_flush = 0; return SB_BLOCK_IO_ERROR; }
    return SB_BLOCK_OK;
}

static sb_block_device_t device = {
    .name = "fat32-write-fixture", .sector_size = SB_BLOCK_SECTOR_SIZE,
    .sector_count = SECTORS, .read = read_disk, .write = write_disk, .flush = flush_disk,
};

/* Fragmented file chain 3 -> 5 -> 6, two sectors per cluster. */
static uint32_t file_position(uint32_t offset) {
    const uint32_t clusters[] = {3u, 5u, 6u};
    return (3u + (clusters[offset / 1024u] - 2u) * 2u) * 512u + offset % 1024u;
}

static void dirent(uint32_t sector, uint32_t slot, const char *name,
                    uint8_t attributes, uint32_t cluster, uint32_t size) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attributes;
    p[26] = (uint8_t)cluster;
    put32(p + 28u, size);
}

static void fixture(void) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 2u; disk[14] = 1u; disk[16] = 2u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    uint8_t *fat = disk + 512u;
    for (uint32_t i = 0u; i < 9u; ++i) put32(fat + i * 4u, 0x0FFFFFFFu);
    put32(fat + 12u, 5u); put32(fat + 20u, 6u);
    memcpy(disk + 1024u, fat, 512u);
    dirent(3u, 0u, "DATA    BIN", 0x20u, 3u, FILE_BYTES);
    dirent(3u, 1u, "LOCKED  BIN", SB_FAT32_ATTR_READ_ONLY, 4u, 512u);
    dirent(3u, 2u, "NEST       ", SB_FAT32_ATTR_DIRECTORY, 7u, 0u);
    dirent(13u, 0u, "INNER   BIN", 0x20u, 8u, 700u);
    for (uint32_t i = 0u; i < FILE_BYTES; ++i) disk[file_position(i)] = (uint8_t)(i ^ 0x39u);
    memset(disk + 7u * 512u, 0x55, 1024u);
    memset(disk + 15u * 512u, 0x77, 1024u);
    memcpy(expected, disk, sizeof(disk));
    sb_block_cache_reset();
}

int main(void) {
    fixture();
    sb_vfs_mount_t mount;
    sb_fat32_vfs_t adapter;
    sb_vfs_node_t *node;
    sb_vfs_file_t file;
    sb_vfs_file_t reader;
    uint64_t written;
    uint8_t payload[1600u];
    uint8_t buffer[1600u];
    for (uint32_t i = 0u; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0xA5u);
    if (require(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
                sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "DATA.BIN", 8u, &node) ==
                    SB_VFS_OBJECT_OK &&
                sb_vfs_file_open(node, SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_open(node, SB_VFS_ACCESS_READ, &reader) == SB_VFS_OBJECT_OK,
                "writable and read-only handles opened")) return 1;
    if (require(sb_vfs_file_write(&reader, payload, 1u, &written) == SB_VFS_OBJECT_ACCESS &&
                reader.offset == 0u && written == 0u, "read-only handle rejected")) return 1;
    if (require(sb_vfs_file_seek(&file, FILE_BYTES + 1u) == SB_VFS_OBJECT_RANGE &&
                file.offset == 0u && sb_block_cache_stats().dirty_writes == 0u,
                "seeking a hole is rejected before mutation")) return 1;
    if (require(sb_vfs_file_seek(&file, 480u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_write(&file, payload, sizeof(payload), &written) == SB_VFS_OBJECT_OK &&
                written == sizeof(payload) && file.offset == 2080u && node->size == FILE_BYTES &&
                memcmp(disk, expected, sizeof(disk)) == 0, "fragmented cross-sector write deferred")) return 1;
    for (uint32_t i = 0u; i < sizeof(payload); ++i) expected[file_position(480u + i)] = payload[i];
    if (require(sb_vfs_file_seek(&reader, 480u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_read(&reader, buffer, sizeof(buffer), &written) == SB_VFS_OBJECT_OK &&
                written == sizeof(buffer) && memcmp(buffer, payload, sizeof(buffer)) == 0,
                "another handle observes dirty data")) return 1;
    fail_write = 1;
    if (require(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO && flushes == 0u,
                "writeback error reaches file sync")) return 1;
    fail_flush = 1;
    if (require(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO && flushes == 1u,
                "device barrier error reaches file sync")) return 1;
    if (require(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && flushes == 2u &&
                memcmp(disk, expected, sizeof(disk)) == 0,
                "sync preserves every byte outside the requested file range")) return 1;
    sb_block_cache_reset();
    if (require(sb_vfs_file_seek(&reader, 480u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_read(&reader, buffer, sizeof(buffer), &written) == SB_VFS_OBJECT_OK &&
                memcmp(buffer, payload, sizeof(buffer)) == 0, "read after cache reset persists")) return 1;

    /* A failure in the second cluster reports only accepted progress. */
    sb_block_cache_reset();
    fail_read_lba = 9;
    if (require(sb_vfs_file_seek(&file, 500u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_write(&file, payload, 1000u, &written) == SB_VFS_OBJECT_OK &&
                written == 524u && file.offset == 1024u, "partial I/O preserves progress and offset")) return 1;
    if (require(sb_vfs_file_write(&file, payload + 524u, 476u, &written) == SB_VFS_OBJECT_OK &&
                written == 476u && file.offset == 1500u &&
                sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK, "short write can be retried")) return 1;
    for (uint32_t i = 0u; i < 1000u; ++i) expected[file_position(500u + i)] = payload[i];
    if (require(memcmp(disk, expected, sizeof(disk)) == 0, "short-write retry changes only intended bytes")) return 1;

    sb_vfs_node_t *directory;
    sb_vfs_node_t *nested_node;
    sb_vfs_file_t nested;
    if (require(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "NEST", 4u, &directory) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_lookup(directory, "INNER.BIN", 9u, &nested_node) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_open(nested_node, SB_VFS_ACCESS_WRITE, &nested) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_seek(&nested, 510u) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_write(&nested, payload, 20u, &written) == SB_VFS_OBJECT_OK &&
                written == 20u && sb_vfs_file_sync(&nested) == SB_VFS_OBJECT_OK,
                "nested file overwrite crosses a sector boundary")) return 1;
    memcpy(expected + 15u * 512u + 510u, payload, 20u);
    if (require(memcmp(disk, expected, sizeof(disk)) == 0 &&
                sb_vfs_file_close(&nested) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_release(nested_node) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_release(directory) == SB_VFS_OBJECT_OK,
                "nested overwrite preserves its directory and surrounding data")) return 1;

    /* A cycle and a prematurely terminated chain must be refused before data changes. */
    const uint32_t invalid_links[] = {3u, 0x0FFFFFFFu, 0u, 0x0FFFFFF7u};
    for (uint32_t i = 0u; i < sizeof(invalid_links) / sizeof(invalid_links[0]); ++i) {
        put32(disk + 512u + 20u, invalid_links[i]);
        sb_block_cache_reset();
        if (require(sb_vfs_file_seek(&file, 0u) == SB_VFS_OBJECT_OK &&
                    sb_vfs_file_write(&file, payload, 1u, &written) == SB_VFS_OBJECT_IO &&
                    written == 0u && file.offset == 0u &&
                    sb_block_cache_stats().dirty_writes == 0u,
                    "malformed chain rejected before mutation")) return 1;
    }
    put32(disk + 512u + 20u, 6u);
    sb_block_cache_reset();
    fail_read_lba = 5;
    if (require(sb_vfs_file_write(&file, payload, 1u, &written) == SB_VFS_OBJECT_IO &&
                written == 0u && file.offset == 0u, "first-sector failure has no progress")) return 1;
    sb_fat32_dirent_t invalid_entry = { .first_cluster = 2u, .file_size = 512u };
    if (require(sb_fat32_write_file(&adapter.fs, &invalid_entry, 0u, 1u, payload, &written) ==
                    SB_VFS_OBJECT_IO && written == 0u,
                "root directory cluster cannot be overwritten as a file")) return 1;
    sb_vfs_node_t *locked;
    if (require(sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "LOCKED.BIN", 10u, &locked) ==
                    SB_VFS_OBJECT_OK &&
                sb_vfs_file_open(locked, SB_VFS_ACCESS_WRITE, &reader) == SB_VFS_OBJECT_ACCESS,
                "FAT read-only attribute enforced")) return 1;
    if (require(sb_vfs_node_release(locked) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_close(&reader) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_release(node) == SB_VFS_OBJECT_OK &&
                sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK,
                "handles and node references released")) return 1;
    sb_block_device_t readonly_device = device;
    readonly_device.write = 0;
    readonly_device.flush = 0;
    if (require(sb_vfs_mount(&readonly_device, &mount) == SB_VFS_OK &&
                sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
                sb_vfs_node_lookup(sb_fat32_vfs_root(&adapter), "DATA.BIN", 8u, &node) == SB_VFS_OBJECT_OK &&
                sb_vfs_file_open(node, SB_VFS_ACCESS_WRITE, &file) == SB_VFS_OBJECT_ACCESS &&
                sb_vfs_node_release(node) == SB_VFS_OBJECT_OK &&
                sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK,
                "read-only block device does not expose write capability")) return 1;
    puts("fat32 write host test OK");
    return 0;
}
