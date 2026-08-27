#ifndef SB_BLOCK_H
#define SB_BLOCK_H

#include <stdint.h>

#define SB_BLOCK_SECTOR_SIZE 512u

typedef enum {
    SB_BLOCK_OK = 0,
    SB_BLOCK_INVALID_ARGUMENT = 1,
    SB_BLOCK_NOT_READY = 2,
} sb_block_status_t;

typedef struct sb_block_device {
    const char *name;
    uint64_t sector_count;
    uint32_t sector_size;
    sb_block_status_t (*read)(struct sb_block_device *device,
                              uint64_t lba,
                              uint32_t count,
                              void *buffer);
    sb_block_status_t (*write)(struct sb_block_device *device,
                               uint64_t lba,
                               uint32_t count,
                               const void *buffer);
    void *driver_data;
} sb_block_device_t;

sb_block_status_t sb_block_register(sb_block_device_t *device);
sb_block_device_t *sb_block_get(uint32_t index);
uint32_t sb_block_count(void);

#endif
