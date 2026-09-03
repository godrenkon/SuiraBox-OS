#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../kernel/fs_syscall.h"
#include "../kernel/syscall.h"
#include "../kernel/process.h"
#include "../kernel/mm/address_space.h"
#include "../kernel/fs/fat32.h"

static sb_process_t process;
static sb_fat32_t fake_fs;
static sb_fat32_dirent_t fake_entry;
static sb_fat32_dirent_t fake_root_entries[2];
static sb_fat32_dirent_t fake_data_entries[1];
static uintptr_t user_base = 0x8000000000ull;
static size_t user_size = 0x20000u;
static uint8_t fake_file[128];
static uint32_t fake_created_size;
static char fake_created_name[13];

sb_process_t *user_scheduler_current_process(void) { return &process; }
int address_space_validate_user_range(const sb_address_space_t *space, uint64_t address, uint64_t size, uint8_t write_access) {
    (void)space;
    (void)write_access;
    if (address < user_base || size == 0u || address > UINT64_MAX - size || address + size > user_base + user_size) return -1;
    return 0;
}
sb_fat32_t *sb_storage_fat32(void) { return &fake_fs; }
sb_block_status_t sb_storage_sync(void) { return SB_BLOCK_OK; }
int sb_fat32_read_root_entry(sb_fat32_t *fs, uint32_t index, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || entry == 0 || index != 0u) return 0;
    *entry = fake_entry;
    return 1;
}
int sb_fat32_find_root_entry(sb_fat32_t *fs, const char *name, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || name == 0 || entry == 0 || strcmp(name, "HELLO.TXT") != 0) return 0;
    *entry = fake_entry;
    return 1;
}
int sb_fat32_lookup_path(sb_fat32_t *fs, const char *path, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || path == 0 || entry == 0) return 0;
    if (strcmp(path, "/HELLO.TXT") == 0) {
        *entry = fake_entry;
        return 1;
    }
    if (strcmp(path, "/DATA") == 0) {
        memset(entry, 0, sizeof(*entry));
        strcpy(entry->name, "DATA");
        entry->attributes = SB_FAT32_ATTR_DIRECTORY;
        entry->first_cluster = 4u;
        return 1;
    }
    return 0;
}
int sb_fat32_read_directory_entry(sb_fat32_t *fs, uint32_t directory_cluster,
                                  uint32_t index, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || entry == 0) return 0;
    if (directory_cluster == fake_fs.root_cluster) {
        if (index >= 2u) return 0;
        *entry = fake_root_entries[index];
        return 1;
    }
    if (directory_cluster == 4u) {
        if (index >= 1u) return 0;
        *entry = fake_data_entries[index];
        return 1;
    }
    return 0;
}
int sb_fat32_create_root_file(sb_fat32_t *fs, const char *name, uint32_t file_size, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || name == 0 || entry == 0 || strcmp(name, "NEW.TXT") != 0) return 0;
    fake_created_size = file_size;
    strcpy(fake_created_name, name);
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->name, name);
    entry->file_size = file_size;
    entry->attributes = 0x20u;
    entry->first_cluster = 2u;
    return 1;
}
int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry, uint32_t offset, uint32_t size, void *buffer) {
    static const char payload[] = "hello";
    const uintptr_t address = (uintptr_t)buffer;
    (void)entry;
    if (fs != &fake_fs || buffer == 0 || offset > 5u || size > 5u - offset) return 0;
    assert(address < user_base || address >= user_base + user_size);
    memcpy(buffer, payload + offset, size);
    return 1;
}
int sb_fat32_write_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry, uint32_t offset, uint32_t size, const void *buffer) {
    const uintptr_t address = (uintptr_t)buffer;
    if (fs != &fake_fs || entry == 0 || buffer == 0 || offset > sizeof(fake_file) || size > sizeof(fake_file) - offset) return 0;
    assert(address < user_base || address >= user_base + user_size);
    memcpy(fake_file + offset, buffer, size);
    return 1;
}
int sb_fat32_write_file_grow(sb_fat32_t *fs, sb_fat32_dirent_t *entry, uint32_t offset, uint32_t size, const void *buffer) {
    if (fs != &fake_fs || entry == 0 || buffer == 0 || offset > sizeof(fake_file) || size > sizeof(fake_file) - offset) return 0;
    if (offset > entry->file_size) return 0;
    if (offset + size > entry->file_size) entry->file_size = offset + size;
    if (!sb_fat32_write_file(fs, entry, offset, size, buffer)) return 0;
    fake_entry = *entry;
    return 1;
}

static void *map_user(void) {
    void *mapped = mmap((void *)user_base, user_size, PROT_READ | PROT_WRITE,
#ifdef MAP_FIXED_NOREPLACE
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
#else
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
#endif
                        -1, 0);
    if (mapped == MAP_FAILED) return 0;
    return mapped;
}

int main(void) {
    char *buffer;
    char *payload;
    void *mapped = map_user();
    assert(mapped == (void *)user_base);
    process.address_space.pml4_physical = 1u;
    fake_fs.root_cluster = 2u;
    memset(&fake_entry, 0, sizeof(fake_entry));
    memcpy(fake_entry.name, "HELLO.TXT", 9u);
    fake_entry.file_size = 5u;
    fake_entry.attributes = 0x20u;

    memset(fake_root_entries, 0, sizeof(fake_root_entries));
    fake_root_entries[0] = fake_entry;
    strcpy(fake_root_entries[1].name, "DATA");
    fake_root_entries[1].attributes = SB_FAT32_ATTR_DIRECTORY;
    fake_root_entries[1].first_cluster = 4u;
    memset(fake_data_entries, 0, sizeof(fake_data_entries));
    strcpy(fake_data_entries[0].name, "NESTED.TXT");
    fake_data_entries[0].attributes = 0x20u;
    fake_data_entries[0].first_cluster = 5u;
    fake_data_entries[0].file_size = 42u;

    buffer = (char *)user_base + 0x1000u;
    payload = buffer + 0x200u;
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_LIST_ROOT, (uint64_t)(uintptr_t)buffer, 64u, 0u, 0u, 0u) == 10u);
    assert(strcmp(buffer, "HELLO.TXT") == 0);
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_STAT_ROOT, (uint64_t)(uintptr_t)buffer, 9u, 0u, 0u, 0u) == ((uint64_t)5u << 32 | 0x20u));
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_READ_ROOT, (uint64_t)(uintptr_t)buffer, 9u,
                                   (uint64_t)(uintptr_t)(buffer + 0x100u), 5u, 0u) == 5u);
    assert(memcmp(buffer + 0x100u, "hello", 5u) == 0);

    assert(sb_fs_syscall_dispatch(SB_SYS_FS_LIST, (uint64_t)(uintptr_t)buffer, 1u,
                                   (uint64_t)(uintptr_t)(buffer + 0x400u), 64u, 0u) == 64u);
    {
        const sb_fs_dir_record_t *records = (const sb_fs_dir_record_t *)(uintptr_t)(buffer + 0x400u);
        assert(records[0].type == SB_FS_DIR_TYPE_FILE);
        assert(records[0].name_length == 9u);
        assert(memcmp(records[0].name, "HELLO.TXT", 9u) == 0);
        assert(records[1].type == SB_FS_DIR_TYPE_DIRECTORY);
        assert(records[1].name_length == 4u);
        assert(memcmp(records[1].name, "DATA", 4u) == 0);
    }
    memcpy(buffer, "/DATA", 5u);
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_LIST, (uint64_t)(uintptr_t)buffer, 5u,
                                   (uint64_t)(uintptr_t)(buffer + 0x400u), 16u, 0u) == 16u);
    {
        const sb_fs_dir_record_t *record = (const sb_fs_dir_record_t *)(uintptr_t)(buffer + 0x400u);
        assert(record->type == SB_FS_DIR_TYPE_FILE);
        assert(record->name_length == 10u);
        assert(memcmp(record->name, "NESTED.TXT", 10u) == 0);
    }

    assert(sb_fs_syscall_dispatch(SB_SYS_FS_LIST, (uint64_t)(uintptr_t)buffer, 1u,
                                   (uint64_t)(uintptr_t)(buffer + 0x400u), 16u, 0u) == UINT64_MAX);
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_LIST, (uint64_t)(uintptr_t)buffer, 5u,
                                   (uint64_t)(uintptr_t)(buffer + 0x400u), 32u, 0u) == UINT64_MAX);

    munmap(mapped, user_size);
    return 0;
}
