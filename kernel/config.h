#ifndef SB_CONFIG_H
#define SB_CONFIG_H

#include <stdint.h>

#define SB_CONFIG_MAGIC   0x53424346u /* "SBCF" */
#define SB_CONFIG_VERSION 1u

typedef enum {
    SB_LANGUAGE_JAPANESE = 0u,
    SB_LANGUAGE_ENGLISH  = 1u,
    SB_LANGUAGE_CHINESE  = 2u,
    SB_LANGUAGE_SPANISH  = 3u,
} sb_language_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t sequence;
    uint8_t language;
    uint8_t keyboard_layout;
    uint8_t first_boot_completed;
    uint8_t reserved;
    uint32_t checksum;
} sb_config_record_t;

void sb_config_defaults(sb_config_record_t *record);
int sb_config_validate(const sb_config_record_t *record);
int sb_config_set_language(sb_config_record_t *record, sb_language_t language);
int sb_config_mark_first_boot_complete(sb_config_record_t *record);
uint32_t sb_config_checksum(const sb_config_record_t *record);

#endif
