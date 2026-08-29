#include "config.h"

static uint32_t fnv1a(const uint8_t *bytes, uint32_t count) {
    uint32_t hash = 2166136261u;
    uint32_t i;
    for (i = 0u; i < count; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

uint32_t sb_config_checksum(const sb_config_record_t *record) {
    sb_config_record_t copy;
    if (record == 0) return 0u;
    copy = *record;
    copy.checksum = 0u;
    return fnv1a((const uint8_t *)&copy, SB_CONFIG_RECORD_SIZE);
}

int sb_config_make(sb_config_record_t *record, sb_language_t language, uint32_t generation) {
    if (record == 0 || language > SB_LANGUAGE_SPANISH) return -1;
    *record = (sb_config_record_t){0};
    record->magic = SB_CONFIG_MAGIC;
    record->version = SB_CONFIG_VERSION;
    record->language = (uint8_t)language;
    record->completed = SB_CONFIG_COMPLETED;
    record->generation = generation;
    record->optional_enabled_mask = 0u;
    record->checksum = sb_config_checksum(record);
    return 0;
}

int sb_config_validate(const sb_config_record_t *record) {
    if (record == 0) return -1;
    if (record->magic != SB_CONFIG_MAGIC) return -1;
    if (record->version != SB_CONFIG_VERSION) return -1;
    if (record->language > SB_LANGUAGE_SPANISH) return -1;
    if (record->completed != SB_CONFIG_COMPLETED) return -1;
    if ((record->optional_enabled_mask & ~SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED) != 0u) return -1;
    if (record->checksum != sb_config_checksum(record)) return -1;
    return 0;
}
