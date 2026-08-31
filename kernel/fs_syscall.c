#include "fs_syscall.h"
#include "syscall.h"
#include "process.h"
#include "user_scheduler.h"
#include "mm/address_space.h"
#include "storage.h"
#include "fs/fat32.h"
#include "vfs.h"
#include <stdint.h>

#define SB_FS_NAME_MAX 12u
#define SB_FS_LIST_MAX_ENTRIES 4096u
#define SB_FS_IO_CHUNK 512u
#define SB_FS_MAX_HANDLES_PER_PROCESS 16u

struct sb_fs_handle {
    sb_process_t *owner;
    sb_fat32_dirent_t entry;
    uint32_t offset;
    uint32_t flags;
    uint8_t in_use;
};

static struct sb_fs_handle g_handles[SB_MAX_PROCESSES][SB_FS_MAX_HANDLES_PER_PROCESS];

static sb_process_t *current_process(void) { return user_scheduler_current_process(); }

static int validate_user(const void *address, uint64_t size, uint8_t write_access) {
    sb_process_t *process = current_process();
    if (process == 0 || address == 0 || size == 0u) return -1;
    return address_space_validate_user_range(&process->address_space,
                                             (uint64_t)(uintptr_t)address, size, write_access);
}

static int copy_from_user(void *destination, const void *source, uint32_t size) {
    if (destination == 0 || source == 0 || size == 0u || validate_user(source, size, 0u) != 0) return -1;
    for (uint32_t i = 0u; i < size; ++i) ((uint8_t *)destination)[i] = ((const uint8_t *)source)[i];
    return 0;
}

static int copy_to_user(void *destination, const void *source, uint32_t size) {
    if (destination == 0 || source == 0 || size == 0u || validate_user(destination, size, 1u) != 0) return -1;
    for (uint32_t i = 0u; i < size; ++i) ((uint8_t *)destination)[i] = ((const uint8_t *)source)[i];
    return 0;
}

static int copy_user_name(const char *user_name, uint32_t name_length, char out[SB_FS_NAME_MAX + 1u]) {
    if (user_name == 0 || name_length == 0u || name_length > SB_FS_NAME_MAX) return -1;
    if (copy_from_user(out, user_name, name_length) != 0) return -1;
    out[name_length] = '\0';
    return 0;
}

static uint64_t list_root(char *user_buffer, uint32_t capacity) {
    sb_fat32_t *fs = sb_storage_fat32();
    uint32_t written = 0u;
    char temp[SB_FS_NAME_MAX + 1u];
    if (fs == 0 || user_buffer == 0 || capacity == 0u || validate_user(user_buffer, capacity, 1u) != 0) return UINT64_MAX;
    for (uint32_t index = 0u; index < SB_FS_LIST_MAX_ENTRIES; ++index) {
        sb_fat32_dirent_t entry;
        if (!sb_fat32_read_root_entry(fs, index, &entry)) continue;
        uint32_t length = 0u;
        while (length < SB_FS_NAME_MAX && entry.name[length] != '\0') ++length;
        if (length == 0u || length + 1u > capacity - written) { if (length != 0u) return UINT64_MAX; continue; }
        for (uint32_t i = 0u; i < length; ++i) temp[i] = entry.name[i];
        temp[length] = '\0';
        if (copy_to_user(user_buffer + written, temp, length + 1u) != 0) return UINT64_MAX;
        written += length + 1u;
    }
    if (written < capacity) {
        const char terminator = '\0';
        if (copy_to_user(user_buffer + written, &terminator, 1u) != 0) return UINT64_MAX;
    }
    return written;
}

static uint64_t stat_root(const char *user_name, uint32_t name_length) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_fat32_dirent_t entry;
    char name[SB_FS_NAME_MAX + 1u];
    if (fs == 0 || copy_user_name(user_name, name_length, name) != 0) return UINT64_MAX;
    if (!sb_fat32_find_root_entry(fs, name, &entry)) return UINT64_MAX;
    return ((uint64_t)entry.file_size << 32) | entry.attributes;
}

static uint64_t read_root(const char *user_name, uint32_t name_length,
                          void *user_buffer, uint32_t capacity, uint32_t offset) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_fat32_dirent_t entry;
    char name[SB_FS_NAME_MAX + 1u];
    uint8_t chunk[SB_FS_IO_CHUNK];
    uint32_t remaining;
    uint32_t current_offset;
    uint8_t *destination;

    if (fs == 0 || user_buffer == 0 || capacity == 0u || copy_user_name(user_name, name_length, name) != 0) return UINT64_MAX;
    if (validate_user(user_buffer, capacity, 1u) != 0) return UINT64_MAX;
    if (!sb_fat32_find_root_entry(fs, name, &entry) || offset > entry.file_size) return UINT64_MAX;
    if (capacity > entry.file_size - offset) capacity = entry.file_size - offset;
    if (capacity == 0u) return 0u;

    remaining = capacity;
    current_offset = offset;
    destination = (uint8_t *)(uintptr_t)user_buffer;
    while (remaining > 0u) {
        const uint32_t count = remaining > SB_FS_IO_CHUNK ? SB_FS_IO_CHUNK : remaining;
        if (!sb_fat32_read_file(fs, &entry, current_offset, count, chunk)) return UINT64_MAX;
        if (copy_to_user(destination, chunk, count) != 0) return UINT64_MAX;
        destination += count;
        current_offset += count;
        remaining -= count;
    }
    return capacity;
}

static uint64_t create_root(const char *user_name, uint32_t name_length, uint32_t file_size) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_fat32_dirent_t entry;
    char name[SB_FS_NAME_MAX + 1u];
    if (fs == 0 || copy_user_name(user_name, name_length, name) != 0) return UINT64_MAX;
    if (sb_fat32_find_root_entry(fs, name, &entry)) return UINT64_MAX;
    if (!sb_fat32_create_root_file(fs, name, file_size, &entry)) return UINT64_MAX;
    return sb_storage_sync() == SB_BLOCK_OK ? 0u : UINT64_MAX;
}

static uint64_t write_root(const char *user_name, uint32_t name_length,
                           const void *user_buffer, uint32_t length, uint32_t offset) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_fat32_dirent_t entry;
    char name[SB_FS_NAME_MAX + 1u];
    uint8_t chunk[SB_FS_IO_CHUNK];
    const uint8_t *source = (const uint8_t *)(uintptr_t)user_buffer;
    uint32_t remaining = length;
    uint32_t current_offset = offset;

    if (fs == 0 || user_buffer == 0 || length == 0u || copy_user_name(user_name, name_length, name) != 0) return UINT64_MAX;
    if (validate_user(user_buffer, length, 0u) != 0) return UINT64_MAX;
    if (!sb_fat32_find_root_entry(fs, name, &entry)) return UINT64_MAX;
    if (offset > entry.file_size || length > entry.file_size - offset) return UINT64_MAX;
    while (remaining > 0u) {
        const uint32_t count = remaining > SB_FS_IO_CHUNK ? SB_FS_IO_CHUNK : remaining;
        if (copy_from_user(chunk, source, count) != 0) return UINT64_MAX;
        if (!sb_fat32_write_file(fs, &entry, current_offset, count, chunk)) return UINT64_MAX;
        source += count;
        current_offset += count;
        remaining -= count;
    }
    return sb_storage_sync() == SB_BLOCK_OK ? (uint64_t)length : UINT64_MAX;
}

static struct sb_fs_handle *get_handle(sb_process_t *process, uint64_t fd) {
    if (process == 0 || fd >= SB_FS_MAX_HANDLES_PER_PROCESS) return 0;
    struct sb_fs_handle *handle = &g_handles[process - (sb_process_t *)0][0];
    (void)handle;
    return &g_handles[process->pid % SB_MAX_PROCESSES][fd];
}

static int process_handle_slot(sb_process_t *process) {
    if (process == 0) return -1;
    return (int)(process->pid % SB_MAX_PROCESSES);
}

static uint64_t open_file(const char *user_path, uint32_t path_length,
                          uint32_t flags, uint32_t initial_size) {
    sb_process_t *process = current_process();
    sb_fat32_t *fs = sb_storage_fat32();
    char path[SB_VFS_MAX_PATH];
    char parent[SB_VFS_MAX_PATH];
    char name[SB_VFS_MAX_NAME + 1u];
    sb_fat32_dirent_t entry;
    sb_vfs_status_t status;
    int slot_index;
    if (process == 0 || fs == 0 || user_path == 0 || path_length == 0u || path_length >= sizeof(path) ||
        (flags & (SB_FS_OPEN_READ | SB_FS_OPEN_WRITE)) == 0u || flags & ~0x07u) return UINT64_MAX;
    if (copy_from_user(path, user_path, path_length) != 0) return UINT64_MAX;
    path[path_length] = '\0';
    status = sb_vfs_split_path(path, parent, sizeof(parent), name, sizeof(name));
    if (status != SB_VFS_OK || parent[0] != '/' || parent[1] != '\0') return UINT64_MAX;
    if (!sb_fat32_find_root_entry(fs, name, &entry)) {
        if ((flags & SB_FS_OPEN_CREATE) == 0u || !sb_fat32_create_root_file(fs, name, initial_size, &entry)) return UINT64_MAX;
        if (sb_storage_sync() != SB_BLOCK_OK) return UINT64_MAX;
    }
    slot_index = process_handle_slot(process);
    if (slot_index < 0) return UINT64_MAX;
    for (uint32_t fd = 0u; fd < SB_FS_MAX_HANDLES_PER_PROCESS; ++fd) {
        struct sb_fs_handle *handle = &g_handles[slot_index][fd];
        if (handle->in_use != 0u) continue;
        *handle = (struct sb_fs_handle){.owner = process, .entry = entry, .offset = 0u, .flags = flags, .in_use = 1u};
        return fd;
    }
    return UINT64_MAX;
}

static uint64_t read_file_handle(uint64_t fd, void *user_buffer, uint32_t length) {
    sb_process_t *process = current_process();
    sb_fat32_t *fs = sb_storage_fat32();
    uint8_t chunk[SB_FS_IO_CHUNK];
    uint8_t *destination = (uint8_t *)(uintptr_t)user_buffer;
    uint32_t remaining = length;
    uint32_t offset;
    struct sb_fs_handle *handle;
    if (process == 0 || fs == 0 || user_buffer == 0 || length == 0u || validate_user(user_buffer, length, 1u) != 0) return UINT64_MAX;
    handle = get_handle(process, fd);
    if (handle == 0 || handle->owner != process || handle->in_use == 0u || (handle->flags & SB_FS_OPEN_READ) == 0u) return UINT64_MAX;
    offset = handle->offset;
    if (offset >= handle->entry.file_size) return 0u;
    if (length > handle->entry.file_size - offset) length = handle->entry.file_size - offset;
    remaining = length;
    while (remaining > 0u) {
        const uint32_t count = remaining > SB_FS_IO_CHUNK ? SB_FS_IO_CHUNK : remaining;
        if (!sb_fat32_read_file(fs, &handle->entry, offset, count, chunk)) return UINT64_MAX;
        if (copy_to_user(destination, chunk, count) != 0) return UINT64_MAX;
        destination += count;
        offset += count;
        remaining -= count;
    }
    handle->offset = offset;
    return length;
}

static uint64_t write_file_handle(uint64_t fd, const void *user_buffer, uint32_t length) {
    sb_process_t *process = current_process();
    sb_fat32_t *fs = sb_storage_fat32();
    uint8_t chunk[SB_FS_IO_CHUNK];
    const uint8_t *source = (const uint8_t *)(uintptr_t)user_buffer;
    uint32_t remaining = length;
    uint32_t offset;
    struct sb_fs_handle *handle;
    if (process == 0 || fs == 0 || user_buffer == 0 || length == 0u || validate_user(user_buffer, length, 0u) != 0) return UINT64_MAX;
    handle = get_handle(process, fd);
    if (handle == 0 || handle->owner != process || handle->in_use == 0u || (handle->flags & SB_FS_OPEN_WRITE) == 0u) return UINT64_MAX;
    offset = handle->offset;
    if (offset > handle->entry.file_size || length > handle->entry.file_size - offset) return UINT64_MAX;
    while (remaining > 0u) {
        const uint32_t count = remaining > SB_FS_IO_CHUNK ? SB_FS_IO_CHUNK : remaining;
        if (copy_from_user(chunk, source, count) != 0) return UINT64_MAX;
        if (!sb_fat32_write_file(fs, &handle->entry, offset, count, chunk)) return UINT64_MAX;
        source += count;
        offset += count;
        remaining -= count;
    }
    if (sb_storage_sync() != SB_BLOCK_OK) return UINT64_MAX;
    handle->offset = offset;
    return length;
}

static uint64_t close_file_handle(uint64_t fd) {
    sb_process_t *process = current_process();
    struct sb_fs_handle *handle;
    if (process == 0) return UINT64_MAX;
    handle = get_handle(process, fd);
    if (handle == 0 || handle->owner != process || handle->in_use == 0u) return UINT64_MAX;
    *handle = (struct sb_fs_handle){0};
    return 0u;
}

void sb_fs_release_process(void *opaque_process) {
    sb_process_t *process = (sb_process_t *)opaque_process;
    if (process == 0) return;
    const int slot = process_handle_slot(process);
    if (slot < 0) return;
    for (uint32_t fd = 0u; fd < SB_FS_MAX_HANDLES_PER_PROCESS; ++fd) {
        if (g_handles[slot][fd].owner == process) g_handles[slot][fd] = (struct sb_fs_handle){0};
    }
}

uint64_t sb_fs_syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    switch (number) {
        case SB_SYS_FS_LIST_ROOT:
            if (arg0 == 0u || arg1 == 0u || arg1 > UINT32_MAX) return UINT64_MAX;
            return list_root((char *)(uintptr_t)arg0, (uint32_t)arg1);
        case SB_SYS_FS_STAT_ROOT:
            if (arg0 == 0u || arg1 == 0u || arg1 > UINT32_MAX) return UINT64_MAX;
            return stat_root((const char *)(uintptr_t)arg0, (uint32_t)arg1);
        case SB_SYS_FS_READ_ROOT:
            if (arg0 == 0u || arg1 == 0u || arg1 > UINT32_MAX || arg2 == 0u || arg3 == 0u || arg3 > UINT32_MAX || arg4 > UINT32_MAX) return UINT64_MAX;
            return read_root((const char *)(uintptr_t)arg0, (uint32_t)arg1,
                             (void *)(uintptr_t)arg2, (uint32_t)arg3, (uint32_t)arg4);
        case SB_SYS_FS_CREATE_ROOT:
            if (arg0 == 0u || arg1 == 0u || arg1 > UINT32_MAX || arg2 > UINT32_MAX) return UINT64_MAX;
            return create_root((const char *)(uintptr_t)arg0, (uint32_t)arg1, (uint32_t)arg2);
        case SB_SYS_FS_WRITE_ROOT:
            if (arg0 == 0u || arg1 == 0u || arg1 > UINT32_MAX || arg2 == 0u || arg3 == 0u || arg3 > UINT32_MAX || arg4 > UINT32_MAX) return UINT64_MAX;
            return write_root((const char *)(uintptr_t)arg0, (uint32_t)arg1,
                              (const void *)(uintptr_t)arg2, (uint32_t)arg3, (uint32_t)arg4);
        case SB_SYS_FS_OPEN:
            if (arg0 == 0u || arg1 == 0u || arg1 >= SB_VFS_MAX_PATH || arg2 > 0x07u || arg3 > UINT32_MAX) return UINT64_MAX;
            return open_file((const char *)(uintptr_t)arg0, (uint32_t)arg1, (uint32_t)arg2, (uint32_t)arg3);
        case SB_SYS_FS_READ:
            if (arg1 == 0u || arg2 == 0u || arg2 > UINT32_MAX) return UINT64_MAX;
            return read_file_handle(arg0, (void *)(uintptr_t)arg1, (uint32_t)arg2);
        case SB_SYS_FS_WRITE:
            if (arg1 == 0u || arg2 == 0u || arg2 > UINT32_MAX) return UINT64_MAX;
            return write_file_handle(arg0, (const void *)(uintptr_t)arg1, (uint32_t)arg2);
        case SB_SYS_FS_CLOSE:
            return close_file_handle(arg0);
        default:
            return UINT64_MAX;
    }
}
