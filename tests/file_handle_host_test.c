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

static sb_process_t process_a;
static sb_process_t process_b;
static sb_process_t *current;
static sb_fat32_t fake_fs;
static sb_fat32_dirent_t hello_entry;
static uint8_t fake_file[128];
static uint32_t created_count;
static uint32_t grow_count;
static uintptr_t user_base = 0x8000000000ull;
static size_t user_size = 0x20000u;

sb_process_t *user_scheduler_current_process(void) { return current; }
int address_space_validate_user_range(const sb_address_space_t *space, uint64_t address, uint64_t size, uint8_t write_access) {
    (void)space;
    (void)write_access;
    if (address < user_base || size == 0u || address > UINT64_MAX - size || address + size > user_base + user_size) return -1;
    return 0;
}
sb_fat32_t *sb_storage_fat32(void) { return &fake_fs; }
sb_block_status_t sb_storage_sync(void) { return SB_BLOCK_OK; }

int sb_fat32_find_root_entry(sb_fat32_t *fs, const char *name, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || name == 0 || entry == 0) return 0;
    if (strcmp(name, "HELLO.TXT") != 0) return 0;
    *entry = hello_entry;
    return 1;
}

int sb_fat32_lookup_path(sb_fat32_t *fs, const char *path, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || path == 0 || entry == 0) return 0;
    if (strcmp(path, "/HELLO.TXT") == 0 || strcmp(path, "/DATA/HELLO.TXT") == 0) {
        *entry = hello_entry;
        return 1;
    }
    return 0;
}

int sb_fat32_create_root_file(sb_fat32_t *fs, const char *name, uint32_t file_size, sb_fat32_dirent_t *entry) {
    if (fs != &fake_fs || name == 0 || entry == 0 || strcmp(name, "NEW.TXT") != 0) return 0;
    ++created_count;
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->name, name);
    entry->attributes = 0x20u;
    entry->file_size = file_size;
    entry->first_cluster = 2u;
    entry->directory_cluster = fake_fs.root_cluster;
    return 1;
}

int sb_fat32_read_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry, uint32_t offset, uint32_t length, void *buffer) {
    static const char payload[] = "hello";
    assert(fs == &fake_fs);
    assert(entry != 0);
    assert((uintptr_t)buffer < user_base || (uintptr_t)buffer >= user_base + user_size);
    if (offset > 5u || length > 5u - offset) return 0;
    memcpy(buffer, payload + offset, length);
    return 1;
}

int sb_fat32_write_file(sb_fat32_t *fs, const sb_fat32_dirent_t *entry, uint32_t offset, uint32_t length, const void *buffer) {
    assert(fs == &fake_fs);
    assert(entry != 0);
    assert((uintptr_t)buffer < user_base || (uintptr_t)buffer >= user_base + user_size);
    if (offset > sizeof(fake_file) || length > sizeof(fake_file) - offset) return 0;
    memcpy(fake_file + offset, buffer, length);
    return 1;
}

int sb_fat32_write_file_grow(sb_fat32_t *fs, sb_fat32_dirent_t *entry, uint32_t offset, uint32_t length, const void *buffer) {
    assert(fs == &fake_fs);
    assert(entry != 0);
    assert(buffer != 0);
    assert((uintptr_t)buffer < user_base || (uintptr_t)buffer >= user_base + user_size);
    assert(offset <= entry->file_size);
    assert(length <= sizeof(fake_file) - offset);
    ++grow_count;
    memcpy(fake_file + offset, buffer, length);
    entry->file_size = offset + length > entry->file_size ? offset + length : entry->file_size;
    hello_entry.file_size = entry->file_size;
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
    return mapped == MAP_FAILED ? 0 : mapped;
}

int main(void) {
    char *path;
    char *buffer;
    void *mapped = map_user();
    assert(mapped == (void *)user_base);
    process_a.pid = 100u;
    process_b.pid = 200u;
    process_a.address_space.pml4_physical = 1u;
    process_b.address_space.pml4_physical = 1u;
    current = &process_a;
    memset(&hello_entry, 0, sizeof(hello_entry));
    strcpy(hello_entry.name, "HELLO.TXT");
    hello_entry.file_size = 5u;
    hello_entry.attributes = 0x20u;
    hello_entry.first_cluster = 2u;
    hello_entry.directory_cluster = 2u;
    path = (char *)user_base + 0x1000u;
    buffer = (char *)user_base + 0x2000u;
    memcpy(path, "/HELLO.TXT", 10u);

    assert(sb_fs_read(UINT64_MAX, buffer, 1u) == UINT64_MAX);
    assert(sb_fs_close(UINT64_MAX) == UINT64_MAX);
    assert(sb_fs_syscall_dispatch(SB_SYS_FS_OPEN, (uint64_t)(uintptr_t)path, 10u, UINT64_MAX, 0u, 0u) == UINT64_MAX);

    const uint64_t fd0 = sb_fs_open(path, 10u, SB_FS_OPEN_READ, 0u);
    assert(fd0 == 0u);
    assert(sb_fs_read(fd0, buffer, 0u) == 0u);
    assert(sb_fs_read(fd0, buffer, 2u) == 2u);
    assert(memcmp(buffer, "he", 2u) == 0);
    assert(sb_fs_read(fd0, buffer, 3u) == 3u);
    assert(memcmp(buffer, "llo", 3u) == 0);
    assert(sb_fs_read(fd0, buffer, 1u) == 0u);
    assert(sb_fs_write(fd0, buffer, 0u) == UINT64_MAX);
    assert(sb_fs_close(fd0) == 0u);
    assert(sb_fs_close(fd0) == UINT64_MAX);

    memcpy(path, "/DATA/HELLO.TXT", 15u);
    const uint64_t nested_fd = sb_fs_open(path, 15u, SB_FS_OPEN_READ, 0u);
    assert(nested_fd == 0u);
    assert(sb_fs_read(nested_fd, buffer, 5u) == 5u);
    assert(memcmp(buffer, "hello", 5u) == 0);
    assert(sb_fs_close(nested_fd) == 0u);

    memcpy(path, "/HELLO.TXT", 10u);
    const uint64_t fd1 = sb_fs_open(path, 10u, SB_FS_OPEN_READ | SB_FS_OPEN_WRITE, 0u);
    assert(fd1 == 0u);
    memcpy(buffer, "world", 5u);
    assert(sb_fs_write(fd1, buffer, 5u) == 5u);
    assert(grow_count == 1u);
    memcpy(buffer, "XYZ", 3u);
    assert(sb_fs_write(fd1, buffer, 3u) == 3u);
    assert(grow_count == 2u);
    assert(hello_entry.file_size == 8u);
    assert(memcmp(fake_file, "worldXYZ", 8u) == 0);
    assert(sb_fs_close(fd1) == 0u);

    memcpy(path, "/NEW.TXT", 8u);
    assert(sb_fs_open(path, 8u, SB_FS_OPEN_READ, 0u) == UINT64_MAX);
    assert(sb_fs_open(path, 8u, SB_FS_OPEN_READ | SB_FS_OPEN_WRITE | SB_FS_OPEN_CREATE, 16u) == 0u);
    assert(created_count == 1u);
    assert(sb_fs_close(0u) == 0u);

    current = &process_a;
    memcpy(path, "/HELLO.TXT", 10u);
    assert(sb_fs_open(path, 10u, SB_FS_OPEN_READ | SB_FS_OPEN_WRITE, 0u) == 0u);
    sb_fs_release_process(&process_a);
    current = &process_b;
    assert(sb_fs_read(0u, buffer, 1u) == UINT64_MAX);
    assert(sb_fs_close(0u) == UINT64_MAX);

    munmap(mapped, user_size);
    return 0;
}
