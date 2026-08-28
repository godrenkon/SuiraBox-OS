#include "config.h"

static uint32_t checksum_bytes(const uint8_t *bytes, uint32_t size) {
    uint32_t hash = 2166136261u;
    uint32_t i;
    for (i = 0u; i < size; ++i) {
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
    return checksum_bytes((const uint8_t *)&copy, (uint32_t)sizeof(copy));
}

void sb_config_defaults(sb_config_record_t *record) {
    if (record == 0) return;
    record->magic = SB_CONFIG_MAGIC;
    record->version = SB_CONFIG_VERSION;
    record->size = (uint16_t)sizeof(*record);
    record->sequence = 0u;
    record->language = (uint8_t)SB_LANGUAGE_ENGLISH;
    record->keyboard_layout = 0u;
    record->first_boot_completed = 0u;
    record->reserved = 0u;
    record->checksum = sb_config_checksum(record);
}

int sb_config_validate(const sb_config_record_t *record) {
    if (record == 0) return 0;
    if (record->magic != SB_CONFIG_MAGIC) return 0;
    if (record->version != SB_CONFIG_VERSION) return 0;
    if (record->size != (uint16_t)sizeof(*record)) return 0;
    if (record->language > (uint8_t)SB_LANGUAGE_SPANISH) return 0;
    if (record->first_boot_completed > 1u) return 0;
    if (record->reserved != 0u) return 0;
    return record->checksum == sb_config_checksum(record);
}

int sb_config_set_language(sb_config_record_t *record, sb_language_t language) {
    if (record == 0 || language > SB_LANGUAGE_SPANISH) return -1;
    record->language = (uint8_t)language;
    record->checksum = sb_config_checksum(record);
    return 0;
}

int sb_config_mark_first_boot_complete(sb_config_record_t *record) {
    if (record == 0) return -1;
    record->first_boot_completed = 1u;
    record->sequence += 1u;
    record->checksum = sb_config_checksum(record);
    return 0;
}
