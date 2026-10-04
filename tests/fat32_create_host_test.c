#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_cache.h"
#include "fs/fat32.h"
#include "vfs_namespace.h"

#define SECTORS 64u
static uint8_t disk[SECTORS * 512u], before[sizeof(disk)];
static sb_vfs_mount_t mount;
static sb_fat32_vfs_t adapter;
static sb_vfs_namespace_t ns;
static int read_failure = -1, write_failure = -1, barrier_failure;
static uint32_t barriers;
static int marker_order_error;

static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "FAT32 create test failed: %s\n", why);
    return 1;
}
static void put32(uint8_t *p, uint32_t n) {
    for (uint32_t i = 0u; i < 4u; ++i) p[i] = (uint8_t)(n >> (i * 8u));
}
static sb_block_status_t read_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == read_failure) { read_failure = -1; return SB_BLOCK_IO_ERROR; }
    memcpy(p, disk + lba * 512u, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t write_disk(sb_block_device_t *d, uint64_t lba, uint32_t count, const void *p) {
    if (!sb_block_range_valid(d, lba, count)) return SB_BLOCK_INVALID_ARGUMENT;
    if ((int)lba == write_failure) { write_failure = -1; return SB_BLOCK_IO_ERROR; }
    const uint8_t *raw = p;
    if (lba == 3u && raw[480u] == 'C' && (barriers == 0u || disk[6u * 512u] != 0u))
        marker_order_error = 1;
    memcpy(disk + lba * 512u, p, count * 512u); return SB_BLOCK_OK;
}
static sb_block_status_t flush_disk(sb_block_device_t *d) {
    if (d == 0) return SB_BLOCK_INVALID_ARGUMENT;
    ++barriers;
    if (barrier_failure) { barrier_failure = 0; return SB_BLOCK_IO_ERROR; }
    return SB_BLOCK_OK;
}
static sb_block_device_t device = {
    .name = "fat32-create", .sector_size = 512u, .sector_count = SECTORS,
    .read = read_disk, .write = write_disk, .flush = flush_disk,
};
static void dirent(uint32_t sector, uint32_t slot, const char *name, uint8_t attr, uint32_t cluster, uint32_t size) {
    uint8_t *p = disk + sector * 512u + slot * 32u;
    memcpy(p, name, 11u); p[11] = attr; p[26] = (uint8_t)cluster; put32(p + 28u, size);
}
static int start(void) {
    memset(disk, 0, sizeof(disk));
    disk[12] = 2u; disk[13] = 1u; disk[14] = 1u; disk[16] = 2u;
    put32(disk + 32u, SECTORS); put32(disk + 36u, 1u); put32(disk + 44u, 2u);
    disk[510] = 0x55u; disk[511] = 0xAAu;
    for (uint32_t c = 0u; c < 11u; ++c) put32(disk + 512u + c * 4u, 0x0FFFFFFFu);
    put32(disk + 512u + 28u, 0u); put32(disk + 512u + 8u, 5u);
    memcpy(disk + 1024u, disk + 512u, 512u);
    dirent(3u, 0u, "GUARD   BIN", 0x20u, 6u, 100u);
    dirent(3u, 1u, "NEST       ", SB_FAT32_ATTR_DIRECTORY, 3u, 0u);
    dirent(3u, 2u, "LOCKED     ", SB_FAT32_ATTR_DIRECTORY | SB_FAT32_ATTR_READ_ONLY, 4u, 0u);
    memset(disk + 3u * 512u + 96u, 0x77, 32u); disk[3u * 512u + 96u] = 0xE5u;
    dirent(3u, 4u, "TAIL    BIN", 0x20u, 8u, 100u);
    disk[3u * 512u + 160u] = 0x41u; disk[3u * 512u + 171u] = SB_FAT32_ATTR_LONG_NAME;
    dirent(3u, 6u, "LONG    TXT", 0x20u, 9u, 12u);
    dirent(3u, 8u, "GHOST   BIN", 0x20u, 10u, 1u);
    memset(disk + 7u * 512u, 0x55, 512u);
    memcpy(before, disk, sizeof(disk));
    read_failure = write_failure = -1; barrier_failure = 0; barriers = 0u; marker_order_error = 0;
    sb_block_cache_reset();
    sb_vfs_namespace_init(&ns);
    return check(sb_vfs_mount(&device, &mount) == SB_VFS_OK &&
        sb_fat32_vfs_init(&adapter, &mount) == SB_VFS_OBJECT_OK &&
        sb_vfs_namespace_mount(&ns, "/disk", 5u, sb_fat32_vfs_root(&adapter)) == SB_VFS_OBJECT_OK,
        "namespace mounted");
}
static int finish(void) {
    sb_vfs_namespace_destroy(&ns);
    return check(sb_fat32_vfs_destroy(&adapter) == SB_VFS_OBJECT_OK, "all references released");
}
static int create(const char *path, uint32_t access, sb_vfs_file_t *file) {
    return sb_vfs_namespace_create_file(&ns, path, strlen(path), access, file);
}
static void boundary_fixture(void) {
    /* Fill the first fragmented root cluster except its last end marker. */
    for (uint32_t i = 0u; i < 15u; ++i) {
        char name[12] = "F000    BIN";
        name[1] = (char)('0' + i / 10u); name[2] = (char)('0' + i % 10u);
        dirent(3u, i, name, 0x20u, 0u, 0u);
    }
    memset(disk + 3u * 512u + 480u, 0, 32u);
    dirent(6u, 0u, "GHOST   BIN", 0x20u, 10u, 1u);
    memcpy(before, disk, sizeof(disk)); sb_block_cache_reset();
}
int main(void) {
    sb_vfs_file_t file, other;
    uint64_t n;
    uint8_t payload[700u], buffer[700u];
    memset(payload, 0xA5, sizeof(payload));
    if (start()) return 1;
    if (check(create("/disk/newfile.txt", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        file.node->size == 0u && file.offset == 0u &&
        sb_vfs_file_read(&file, buffer, 1u, &n) == SB_VFS_OBJECT_OK && n == 0u &&
        memcmp(disk, before, sizeof(disk)) == 0, "exclusive empty creation stays cached")) return 1;
    const uint64_t dirty = sb_block_cache_stats().dirty_writes;
    if (check(create("/disk/NEWFILE.TXT", SB_VFS_ACCESS_ALL, &other) == SB_VFS_OBJECT_EXISTS &&
        !other.open && sb_block_cache_stats().dirty_writes == dirty,
        "case-insensitive duplicate has no mutation")) return 1;
    if (check(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
        memcmp(disk + 3u * 512u + 96u, "NEWFILE TXT", 11u) == 0 &&
        memcmp(disk + 512u, before + 512u, 1024u) == 0 &&
        memcmp(disk + 3u * 512u + 128u, before + 3u * 512u + 128u, 384u) == 0,
        "deleted slot reused without changing FAT or following entries")) return 1;
    if (check(sb_vfs_file_write(&file, payload, sizeof(payload), &n) == SB_VFS_OBJECT_OK && n == sizeof(payload) &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
        "created file allocates across a cluster boundary")) return 1;
    sb_block_cache_reset();
    if (check(sb_vfs_namespace_open_file(&ns, "/disk/NEWFILE.TXT", 17u, SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_read(&file, buffer, sizeof(buffer), &n) == SB_VFS_OBJECT_OK && n == sizeof(buffer) &&
        memcmp(buffer, payload, sizeof(buffer)) == 0 && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK &&
        memcmp(disk + 7u * 512u, before + 7u * 512u, 512u) == 0,
        "reopened created file persists and guard remains unchanged")) return 1;
    if (check(create("/disk/END.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && disk[3u * 512u + 256u] == 0u &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK, "end-marker successor suppresses hidden garbage")) return 1;
    sb_vfs_node_t *ghost;
    if (check(sb_vfs_namespace_resolve(&ns, "/disk/GHOST.BIN", 15u, &ghost) == SB_VFS_OBJECT_NOT_FOUND,
        "hidden entry cannot become a ghost file")) return 1;
    if (check(create("/disk//NEST/./CHILD.BIN", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_write(&file, payload, 100u, &n) == SB_VFS_OBJECT_OK && sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK &&
        sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK && memcmp(disk + 4u * 512u, "CHILD   BIN", 11u) == 0,
        "normalized nested creation uses correct parent directory")) return 1;
    memcpy(before, disk, sizeof(disk));
    const char *invalid[] = {"/disk/TOOLONG99.TXT", "/disk/A.LONG", "/disk/.TXT", "/disk/A.",
        "/disk/A B.TXT", "/disk/A+B.TXT", "/disk/A..B", "/disk/X/", "/disk/X/.", "/disk/X/..", "relative"};
    for (uint32_t i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        if (check(create(invalid[i], SB_VFS_ACCESS_ALL, &file) != SB_VFS_OBJECT_OK && !file.open,
            "invalid creation name/path rejected")) return 1;
    if (check(create("/disk/READ.TXT", SB_VFS_ACCESS_READ, &file) == SB_VFS_OBJECT_ACCESS &&
        create("/disk/UNKNOWN.TXT", 8u, &file) == SB_VFS_OBJECT_INVALID &&
        create("/disk/MISSING/X.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_NOT_FOUND &&
        create("/disk/LOCKED/X.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
        create("/disk", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_EXISTS &&
        create("/disk/LONG.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_EXISTS &&
        memcmp(disk, before, sizeof(disk)) == 0, "rights, mount root, LFN alias and missing parent protected")) return 1;
    adapter.fs.write_faulted = 1u;
    if (check(create("/disk/FAULT.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_IO, "quarantined mount refuses creation")) return 1;
    if (finish()) return 1;
    /* End marker publication crosses a fragmented directory cluster. */
    for (uint32_t fault = 0u; fault < 4u; ++fault) {
        if (start()) return 1;
        boundary_fixture();
        if (fault == 1u) write_failure = 6;
        if (fault == 2u) barrier_failure = 1;
        if (fault == 3u) read_failure = 6;
        const int result = create("/disk/CROSS.TXT", SB_VFS_ACCESS_ALL, &file);
        if (fault != 0u) {
            if (check(result == SB_VFS_OBJECT_IO && !file.open && disk[3u * 512u + 480u] == 0u,
                "successor-marker failure leaves new file unpublished")) return 1;
            if (check(create("/disk/CROSS.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK,
                "marker failure can be retried")) return 1;
        } else if (check(result == SB_VFS_OBJECT_OK, "cross-cluster creation accepted")) return 1;
        write_failure = 3;
        if (check(sb_vfs_file_sync(&file) == SB_VFS_OBJECT_IO &&
            sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && !marker_order_error &&
            disk[6u * 512u] == 0u && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
            "successor marker durable before publication and FILE_SYNC retries")) return 1;
        if (finish()) return 1;
    }
    if (start()) return 1;
    boundary_fixture();
    for (uint32_t i = 0u; i < 16u; ++i) {
        char name[12] = "Z000    BIN"; name[1] = (char)('0' + i / 10u); name[2] = (char)('0' + i % 10u);
        dirent(6u, i, name, 0x20u, 0u, 0u);
    }
    dirent(3u, 15u, "FINAL   BIN", 0x20u, 0u, 0u);
    memcpy(before, disk, sizeof(disk)); sb_block_cache_reset();
    if (check(create("/disk/FULL.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_OK &&
        file.open && file.node->size == 0u && disk[512u + 20u] == 7u && disk[1024u + 20u] == 7u &&
        memcmp(disk + 3u * 512u, before + 3u * 512u, 512u) == 0 &&
        memcmp(disk + 6u * 512u, before + 6u * 512u, 512u) == 0 &&
        sb_vfs_file_sync(&file) == SB_VFS_OBJECT_OK && sb_vfs_file_close(&file) == SB_VFS_OBJECT_OK,
        "full directory grows while preserving every existing directory slot")) return 1;
    if (finish() || start()) return 1;
    put32(disk + 512u + 20u, 2u); sb_block_cache_reset();
    if (check(create("/disk/CYCLE.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_IO &&
        sb_block_cache_stats().dirty_writes == 0u, "cyclic directory refused before publication")) return 1;
    if (finish() || start()) return 1;
    disk[3u * 512u + 96u] = 0x41u; disk[3u * 512u + 107u] = SB_FAT32_ATTR_LONG_NAME;
    memset(disk + 3u * 512u + 128u, 0, 32u); sb_block_cache_reset();
    if (check(create("/disk/ORPHAN.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_RANGE &&
        sb_block_cache_stats().dirty_writes == 0u, "orphaned LFN cannot attach to a new short name")) return 1;
    if (finish()) return 1;
    if (start()) return 1;
    put32(disk + 512u + 20u, 11u); put32(disk + 512u + 44u, 0x0FFFFFFFu);
    memcpy(disk + 1024u, disk + 512u, 512u);
    memset(disk + 3u * 512u, 0, 512u); memset(disk + 6u * 512u, 0, 512u);
    memset(disk + 12u * 512u, 0, 512u);
    for (uint32_t i = 0u; i < 32u; ++i) {
        char name[12] = "F000    TXT";
        name[1] = (char)('0' + i / 10u); name[2] = (char)('0' + i % 10u);
        dirent(i < 16u ? 3u : 6u, i % 16u, name, 0x20u, 0u, 0u);
    }
    sb_block_cache_reset();
    for (uint32_t i = 0u; i < 32u; ++i) {
        char path[16] = "/disk/F000.TXT";
        path[7] = (char)('0' + i / 10u); path[8] = (char)('0' + i % 10u);
        sb_vfs_node_t *cached;
        if (check(sb_vfs_namespace_resolve(&ns, path, strlen(path), &cached) == SB_VFS_OBJECT_OK &&
            sb_vfs_node_release(cached) == SB_VFS_OBJECT_OK, "real nodes fill bounded cache")) return 1;
    }
    memcpy(before, disk, sizeof(disk));
    if (check(create("/disk/CACHE.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_RANGE &&
        !file.open && sb_block_cache_stats().dirty_writes == 0u && memcmp(disk, before, sizeof(disk)) == 0,
        "node-cache exhaustion cannot publish an unreachable file")) return 1;
    if (finish()) return 1;
    device.write = 0;
    if (start()) return 1;
    if (check(create("/disk/RO.TXT", SB_VFS_ACCESS_ALL, &file) == SB_VFS_OBJECT_ACCESS &&
        sb_block_cache_stats().dirty_writes == 0u, "read-only device cannot create files")) return 1;
    if (finish()) return 1;
    device.write = write_disk;
    puts("fat32 create host test OK");
    return 0;
}
