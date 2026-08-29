#include "config_store.h"
#include "storage.h"
#include "fs/fat32.h"
#include <stdint.h>

#define SB_CONFIG_FILE_NAME "SBCFG.BIN"

static uint32_t fnv1a(const uint8_t *bytes, uint32_t count) {
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0u; i < count; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t checksum(const sb_config_store_record_t *record) {
    sb_config_store_record_t copy;
    if (record == 0) return 0u;
    copy = *record;
    copy.checksum = 0u;
    return fnv1a((const uint8_t *)&copy, SB_CONFIG_STORE_RECORD_SIZE);
}

static int validate(const sb_config_store_record_t *record) {
    if (record == 0 || record->magic != SB_CONFIG_STORE_MAGIC ||
        record->version != SB_CONFIG_STORE_VERSION || record->language > 3u ||
        record->completed != SB_CONFIG_STORE_COMPLETED ||
        record->checksum != checksum(record)) return 0;
    return 1;
}

int sb_config_store_get(sb_config_store_record_t *record) {
    sb_fat32_t *fs;
    sb_fat32_dirent_t entry;
    if (record == 0) return 0;
    fs = sb_storage_fat32();
    if (fs == 0 || !sb_fat32_find_root_entry(fs, SB_CONFIG_FILE_NAME, &entry) ||
        entry.file_size != SB_CONFIG_STORE_RECORD_SIZE ||
        !sb_fat32_read_file(fs, &entry, 0u, SB_CONFIG_STORE_RECORD_SIZE, record)) return 0;
    return validate(record);
}

int sb_config_store_set(uint8_t language) {
    sb_fat32_t *fs;
    sb_fat32_dirent_t entry;
    sb_config_store_record_t current;
    sb_config_store_record_t next = {0};
    uint32_t generation = 1u;

    if (language > 3u) return -1;
    fs = sb_storage_fat32();
    if (fs == 0) return -1;

    if (sb_config_store_get(&current)) {
        if (current.generation == UINT32_MAX) return -1;
        generation = current.generation + 1u;
        if (!sb_fat32_find_root_entry(fs, SB_CONFIG_FILE_NAME, &entry) ||
            entry.file_size != SB_CONFIG_STORE_RECORD_SIZE) return -1;
    } else {
        if (sb_fat32_find_root_entry(fs, SB_CONFIG_FILE_NAME, &entry)) {
            if (entry.file_size != SB_CONFIG_STORE_RECORD_SIZE) return -1;
        } else if (!sb_fat32_create_root_file(fs, SB_CONFIG_FILE_NAME,
                                               SB_CONFIG_STORE_RECORD_SIZE, &entry)) {
            return -1;
        }
    }

    next.magic = SB_CONFIG_STORE_MAGIC;
    next.version = SB_CONFIG_STORE_VERSION;
    next.language = language;
    next.completed = SB_CONFIG_STORE_COMPLETED;
    next.generation = generation;
    next.checksum = checksum(&next);
    if (!sb_fat32_write_file(fs, &entry, 0u, SB_CONFIG_STORE_RECORD_SIZE, &next)) return -1;

    return 0;
}
