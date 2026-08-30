#ifndef SB_KERNEL_NVME_H
#define SB_KERNEL_NVME_H

#include <stdint.h>

#define SB_NVME_MAX_CONTROLLERS 16u

typedef struct {
    uint32_t device_index;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t ready;
} sb_nvme_controller_t;

void sb_nvme_init(void);
uint32_t sb_nvme_controller_count(void);
const sb_nvme_controller_t *sb_nvme_controller_get(uint32_t index);

#endif
