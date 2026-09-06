#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../kernel/fs_syscall.h"
#include "../kernel/syscall.h"
#include "../kernel/process.h"
#include "../kernel/fs/fat32.h"

static sb_process_t process;
static sb_fat32_t fake_fs;
static sb_fat32_dirent_t disk_entry;
static uint8_t file_data[64];
static sb_process_t *current_process;

sb_process_t *user_scheduler_current_process(void) { return current_process; }
int address_space_validate_user_range(const sb_address_space_t *space, uint64_t address,
                                      uint64_t size, uint8_t write_access) {
    (void)space;
    (void)write_access;
    return address != 0u && size != 0u ? 0 : -1;
}
sb_fat32_t *sb_storage_fat32(void) { return &fake_fs; }
sb_block_status_t sb_storage_sync(void) { return SB_BLOCK_OK; }

int sb_vfs_normalize_path(const char *input, char *output, uint32_t capacity) {
    if (input == 0 || output == 0 || capacity == 0u) return SB_VFS_ERR_INVALID;
    const size_t length = strlen(input);
    if (length + 1u > capacity) return SB_VFS_ERR_RANGE;
    memmove(output, input, length + 1u);
    return SB_VFS_OK;
}

sb_vfs_status_t sb_vfs_split_path(const char *path, char *parent, uint32_t parent_capacity,
                                  char *name, uint32_t name_capacity) {
    (void)path;
    (void)parent;
    (void)parent_capacity;
    (void)name;
    (void)name_capacity;
    return SB_VFS_ERR_INVALID;
}

static void copy_entry(sb_fat32_dirent_t *entry) { *entry = disk_entry; }

int sb_fat32_find_root_entry(sb_fat32_t *fs, const char *name, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || name == 0 || entry == 0 || strcmp(name, "SHARED.TXT") != 0) return 0;
    copy_entry(entry);
    return 1;
}

int sb_fat32_lookup_path(sb_fat32_t *fs, const char *path, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || path == 0 || entry == 0 || strcmp(path, "/SHARED.TXT") != 0) return 0;
    copy_entry(entry);
    return 1;
}

int sb_fat32_read_directory_entry(sb_fat32_t *fs, uint32_t directory_cluster,
                                  uint32_t index, sb_fat32_dirent_t *entry) {
    (void)fs; (void)directory_cluster; (void)index; (void)entry;
    return 0;
}

int sb_fat32_create_root_file(sb_fat32_t *fs, const char *name, uint32_t file_size, sb_fat32_dirent_t *entry) {
    (void)fs; (void)name; (void)file_size; (void)entry;
    return 0;
}

int sb_fat32_create_file_in_directory(sb_fat32_t *fs, uint32_t directory_cluster,
                                       const char *name, uint32_t file_size,
                                       sb_fat32_dirent_t *entry) {
    (void)fs; (void)directory_cluster; (void)name; (void)file_size; (void)entry;
    return 0;
}

int sb_fat32_create_directory_in_directory(sb_fat32_t *fs, uint32_t parent_cluster,
                                            const char *name, uint32_t *directory_cluster,
                                            sb_fat32_dirent_t *entry) {
    (void)fs; (void)parent_cluster; (void)name; (void)directory_cluster; (void)entry;
    return 0;
}

int sb_fat32_refresh_dirent(sb_fat32_t *fs, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || entry == 0 || entry->entry_lba != disk_entry.entry_lba ||
        entry->entry_offset != disk_entry.entry_offset) return 0;
    copy_entry(entry);
    return 1;
}

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                       uint32_t offset, uint32_t length, void *buffer) {
    if (fs != &fake_fs || entry == 0 || buffer == 0 ||
        offset > disk_entry.file_size || length > disk_entry.file_size - offset) return 0;
    memcpy(buffer, file_data + offset, length);
    return 1;
}

int sb_fat32_write_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry,
                        uint32_t offset, uint32_t length, const void *buffer) {
    if (fs != &fake_fs || entry == 0 || buffer == 0 || offset > sizeof(file_data) ||
        length > sizeof(file_data) - offset) return 0;
    memcpy(file_data + offset, buffer, length);
    if (offset + length > disk_entry.file_size) disk_entry.file_size = offset + length;
    return 1;
}

int sb_fat32_write_file_grow(sb_fat32_t *fs, sb_fat32_dirent_t *entry,
                             uint32_t offset, uint32_t length, const void *buffer) {
    if (!sb_fat32_write_file(fs, entry, offset, length, buffer)) return 0;
    entry->file_size = disk_entry.file_size;
    entry->first_cluster = disk_entry.first_cluster;
    entry->entry_lba = disk_entry.entry_lba;
    entry->entry_offset = disk_entry.entry_offset;
    return 1;
}

int main(void) {
    char path[] = "/SHARED.TXT";
    char output[9] = {0};
    const char first_data[] = "world";
    const char second_data[] = "XYZ";

    memset(&process, 0, sizeof(process));
    process.pid = 42u;
    process.state = SB_PROCESS_RUNNING;
    process.address_space.pml4_physical = 1u;
    current_process = &process;

    memset(&fake_fs, 0, sizeof(fake_fs));
    memset(&disk_entry, 0, sizeof(disk_entry));
    strcpy(disk_entry.name, "SHARED.TXT");
    disk_entry.attributes = 0x20u;
    disk_entry.first_cluster = 2u;
    disk_entry.file_size = 5u;
    disk_entry.directory_cluster = 2u;
    disk_entry.entry_lba = 100u;
    disk_entry.entry_offset = 0u;
    memcpy(file_data, "hello", 5u);

    const uint64_t fd_a = sb_fs_open(path, 11u, SB_FS_OPEN_READ | SB_FS_OPEN_WRITE, 0u);
    const uint64_t fd_b = sb_fs_open(path, 11u, SB_FS_OPEN_READ | SB_FS_OPEN_WRITE, 0u);
    assert(fd_a == 0u && fd_b == 1u);

    assert(sb_fs_write(fd_a, first_data, 5u) == 5u);
    assert(sb_fs_write(fd_a, second_data, 3u) == 3u);
    assert(disk_entry.file_size == 8u);

    assert(sb_fs_seek(fd_b, 0, SB_FS_SEEK_END) == 8u);
    assert(sb_fs_seek(fd_b, -3, SB_FS_SEEK_END) == 5u);
    assert(sb_fs_read(fd_b, output, 3u) == 3u);
    assert(memcmp(output, second_data, 3u) == 0);
    assert(sb_fs_read(fd_b, output, 1u) == 0u);

    assert(sb_fs_close(fd_a) == 0u);
    assert(sb_fs_close(fd_b) == 0u);
    return 0;
}
