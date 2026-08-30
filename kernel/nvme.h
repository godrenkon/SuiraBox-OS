#ifndef SB_KERNEL_NVME_H
#define SB_KERNEL_NVME_H

#include <stdint.h>

#define SB_NVME_MAX_CONTROLLERS 16u

typedef struct {
    uint32_t device_index;
    uint16_t vendor_id;
    uint16_t device_id;
    uint64_t mmio_base;
    uint32_t version;
    uint32_t max_queue_entries;
    uint16_t max_submission_queue_entries;
    uint16_t max_completion_queue_entries;
    uint32_t controller_status;
    uint8_t ready;
    uint8_t mmio_valid;
} sb_nvme_controller_t;

void sb_nvme_init(void);
uint32_t sb_nvme_controller_count(void);
const sb_nvme_controller_t *sb_nvme_controller_get(uint32_t index);

#endif
