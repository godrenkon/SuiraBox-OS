#include "config_store.h"
#include "storage.h"
#include "fs/fat32.h"
#include <stdint.h>

#define SB_CONFIG_FILE_A "SBCFG.BIN"
#define SB_CONFIG_FILE_B "SBCF2.BIN"

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
        (record->optional_enabled_mask & ~SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED) != 0u ||
        record->checksum != checksum(record)) return 0;
    return 1;
}

static int load_slot(sb_fat32_t *fs, const char *name,
                     sb_config_store_record_t *record, sb_fat32_dirent_t *entry) {
    if (fs == 0 || name == 0 || record == 0 || entry == 0) return 0;
    if (!sb_fat32_find_root_entry(fs, name, entry) ||
        entry->file_size != SB_CONFIG_STORE_RECORD_SIZE ||
        !sb_fat32_read_file(fs, entry, 0u, SB_CONFIG_STORE_RECORD_SIZE, record)) return 0;
    return validate(record);
}

int sb_config_store_get(sb_config_store_record_t *record) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_config_store_record_t a;
    sb_config_store_record_t b;
    sb_fat32_dirent_t entry_a;
    sb_fat32_dirent_t entry_b;
    int valid_a;
    int valid_b;

    if (record == 0 || fs == 0) return 0;
    valid_a = load_slot(fs, SB_CONFIG_FILE_A, &a, &entry_a);
    valid_b = load_slot(fs, SB_CONFIG_FILE_B, &b, &entry_b);
    if (valid_a == 0 && valid_b == 0) return 0;
    if (valid_a != 0 && (valid_b == 0 || a.generation >= b.generation)) *record = a;
    else *record = b;
    return 1;
}

static int ensure_slot(sb_fat32_t *fs, const char *name,
                       sb_fat32_dirent_t *entry) {
    if (sb_fat32_find_root_entry(fs, name, entry))
        return entry->file_size == SB_CONFIG_STORE_RECORD_SIZE ? 0 : -1;
    return sb_fat32_create_root_file(fs, name, SB_CONFIG_STORE_RECORD_SIZE, entry) ? 1 : -1;
}

int sb_config_store_set(uint8_t language, uint32_t optional_enabled_mask) {
    sb_fat32_t *fs = sb_storage_fat32();
    sb_config_store_record_t current_a = {0};
    sb_config_store_record_t current_b = {0};
    sb_fat32_dirent_t entry_a = {0};
    sb_fat32_dirent_t entry_b = {0};
    int valid_a;
    int valid_b;
    sb_fat32_dirent_t *target_entry;
    uint32_t generation = 1u;
    sb_config_store_record_t next = {0};
    int ensure_result;

    if (language > 3u || fs == 0) return -1;
    valid_a = load_slot(fs, SB_CONFIG_FILE_A, &current_a, &entry_a);
    valid_b = load_slot(fs, SB_CONFIG_FILE_B, &current_b, &entry_b);

    if (optional_enabled_mask == SB_CONFIG_SET_KEEP_OPTIONS) {
        if (valid_a != 0 || valid_b != 0) {
            const sb_config_store_record_t *latest =
                (valid_a != 0 && (valid_b == 0 || current_a.generation >= current_b.generation))
                    ? &current_a : &current_b;
            optional_enabled_mask = latest->optional_enabled_mask;
        } else {
            optional_enabled_mask = 0u;
        }
    }
    if ((optional_enabled_mask & ~SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED) != 0u)
        return -1;

    if (valid_a != 0 || valid_b != 0) {
        const sb_config_store_record_t *latest =
            (valid_a != 0 && (valid_b == 0 || current_a.generation >= current_b.generation))
                ? &current_a : &current_b;
        if (latest->generation == UINT32_MAX) return -1;
        generation = latest->generation + 1u;
    }

    if (valid_a == 0) {
        target_entry = &entry_a;
        ensure_result = ensure_slot(fs, SB_CONFIG_FILE_A, target_entry);
    } else if (valid_b == 0) {
        target_entry = &entry_b;
        ensure_result = ensure_slot(fs, SB_CONFIG_FILE_B, target_entry);
    } else if (current_a.generation <= current_b.generation) {
        target_entry = &entry_a;
        ensure_result = 0;
    } else {
        target_entry = &entry_b;
        ensure_result = 0;
    }
    if (ensure_result < 0) return -1;

    next.magic = SB_CONFIG_STORE_MAGIC;
    next.version = SB_CONFIG_STORE_VERSION;
    next.language = language;
    next.completed = SB_CONFIG_STORE_COMPLETED;
    next.generation = generation;
    next.optional_enabled_mask = optional_enabled_mask;
    next.checksum = checksum(&next);
    return sb_fat32_write_file(fs, target_entry, 0u,
                               SB_CONFIG_STORE_RECORD_SIZE, &next) ? 0 : -1;
}
