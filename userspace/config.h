#ifndef SB_CONFIG_H
#define SB_CONFIG_H

#include <stdint.h>

#define SB_CONFIG_MAGIC 0x53424346u /* SBCF */
#define SB_CONFIG_VERSION 1u
#define SB_CONFIG_COMPLETED 1u
#define SB_CONFIG_RECORD_SIZE 32u

typedef enum {
    SB_LANGUAGE_JAPANESE = 0u,
    SB_LANGUAGE_ENGLISH = 1u,
    SB_LANGUAGE_CHINESE = 2u,
    SB_LANGUAGE_SPANISH = 3u,
} sb_language_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint8_t language;
    uint8_t completed;
    uint32_t generation;
    uint32_t checksum;
    uint8_t reserved[16];
} sb_config_record_t;

uint32_t sb_config_checksum(const sb_config_record_t *record);
int sb_config_make(sb_config_record_t *record, sb_language_t language, uint32_t generation);
int sb_config_validate(const sb_config_record_t *record);

#endif
