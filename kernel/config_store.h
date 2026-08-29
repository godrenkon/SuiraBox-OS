#ifndef SB_KERNEL_CONFIG_STORE_H
#define SB_KERNEL_CONFIG_STORE_H

#include <stdint.h>

#define SB_CONFIG_STORE_MAGIC 0x53424346u
#define SB_CONFIG_STORE_VERSION 1u
#define SB_CONFIG_STORE_COMPLETED 1u
#define SB_CONFIG_STORE_RECORD_SIZE 32u
#define SB_CONFIG_OPTIONAL_MASK_ALL_SUPPORTED 0x0000001Fu

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint8_t language;
    uint8_t completed;
    uint32_t generation;
    uint32_t checksum;
    uint32_t optional_enabled_mask;
    uint8_t reserved[12];
} sb_config_store_record_t;

int sb_config_store_get(sb_config_store_record_t *record);
int sb_config_store_set(uint8_t language, uint32_t optional_enabled_mask);

#endif
