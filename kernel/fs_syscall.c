#include "fs_syscall.h"
#include "syscall.h"
#include "process.h"
#include "user_scheduler.h"
#include "mm/address_space.h"
#include "storage.h"
#include "fs/fat32.h"
#include <stdint.h>

#define SB_FS_NAME_MAX 12u
#define SB_FS_LIST_MAX_ENTRIES 4096u

static sb_process_t *current_process(void) {
    return user_scheduler_current_process();
}

static int validate_user(const void *address, uint64_t size, uint8_t write_access) {
    sb_process_t *process = current_process();
    if (process == 0 || address == 0 || size == 0u) return -1;
    return address_space_validate_user_range(&process->address_space,
                                             (uint64_t)(uintptr_t)address,
                                             size,
                                             write_access);
}

static int copy_from_user(char *destination, const char *source, uint32_t size) {
    if (destination == 0 || source == 0 || size == 0u || validate_user(source, size, 0u) != 0) return -1;
    for (uint32_t i = 0u; i < size; ++i) destination[i] = source[i];
    return 0;
}

static int copy_to_user(char *destination, const char *source, uint32_t size) {
    if (destination == 0 || source == 0 || size == 0u || validate_user(destination, size, 1u) != 0) return -1;
    for (uint32_t i = 0u; i < size; ++i) destination[i] = source[i];
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
    if (fs == 0 || user_buffer == 0 || capacity == 0u) return UINT64_MAX;
    if (validate_user(user_buffer, capacity, 1u) != 0) return UINT64_MAX;
    for (uint32_t index = 0u; index < SB_FS_LIST_MAX_ENTRIES; ++index) {
        sb_fat32_dirent_t entry;
        if (!sb_fat32_read_root_entry(fs, index, &entry)) {
            uint8_t sector_end = 0u;
            (void)sector_end;
            continue;
        }
        uint32_t length = 0u;
        while (length < SB_FS_NAME_MAX && entry.name[length] != '\0') ++length;
        if (length == 0u) continue;
        if (written + length + 1u > capacity) return UINT64_MAX;
        for (uint32_t i = 0u; i < length; ++i) temp[i] = entry.name[i];
        temp[length] = '\0';
        if (copy_to_user(user_buffer + written, temp, length + 1u) != 0) return UINT64_MAX;
        written += length + 1u;
    }
    if (written < capacity && validate_user(user_buffer + written, 1u, 1u) == 0)
        user_buffer[written] = '\0';
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
                          char *user_buffer, uint32_t capacity, uint32_t offset) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_fat32_dirent_t entry;
    char name[SB_FS_NAME_MAX + 1u];
    if (fs == 0 || user_buffer == 0 || capacity == 0u || copy_user_name(user_name, name_length, name) != 0) return UINT64_MAX;
    if (validate_user(user_buffer, capacity, 1u) != 0) return UINT64_MAX;
    if (!sb_fat32_find_root_entry(fs, name, &entry)) return UINT64_MAX;
    if (offset > entry.file_size) return UINT64_MAX;
    if (capacity > entry.file_size - offset) capacity = entry.file_size - offset;
    if (capacity == 0u) return 0u;
    if (!sb_fat32_read_file(fs, &entry, offset, capacity, user_buffer)) return UINT64_MAX;
    return capacity;
}

uint64_t sb_fs_syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1,
                                uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    (void)arg4;
    switch (number) {
        case SB_SYS_FS_LIST_ROOT:
            if (arg0 > UINT64_MAX || arg1 > UINT32_MAX) return UINT64_MAX;
            return list_root((char *)(uintptr_t)arg0, (uint32_t)arg1);
        case SB_SYS_FS_STAT_ROOT:
            if (arg0 == 0u || arg1 > UINT32_MAX) return UINT64_MAX;
            return stat_root((const char *)(uintptr_t)arg0, (uint32_t)arg1);
        case SB_SYS_FS_READ_ROOT:
            if (arg0 == 0u || arg1 > UINT32_MAX || arg2 == 0u || arg3 > UINT32_MAX || arg4 > UINT32_MAX)
                return UINT64_MAX;
            return read_root((const char *)(uintptr_t)arg0, (uint32_t)arg1,
                             (char *)(uintptr_t)arg2, (uint32_t)arg3, (uint32_t)arg4);
        default:
            return UINT64_MAX;
    }
}
