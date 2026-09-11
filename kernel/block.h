#ifndef SB_BLOCK_H
#define SB_BLOCK_H

#include <stdint.h>

#define SB_BLOCK_SECTOR_SIZE 512u

typedef enum {
    SB_BLOCK_OK = 0,
    SB_BLOCK_INVALID_ARGUMENT = 1,
    SB_BLOCK_NOT_READY = 2,
    SB_BLOCK_UNSUPPORTED = 3,
    SB_BLOCK_IO_ERROR = 4,
} sb_block_status_t;

typedef struct sb_block_device {
    const char *name;
    uint64_t sector_count;
    uint32_t sector_size;
    sb_block_status_t (*read)(struct sb_block_device *device,
                              uint64_t lba,
                              uint32_t count,
                              void *buffer);
    /* Optional. A null callback means the device is read-only. */
    sb_block_status_t (*write)(struct sb_block_device *device,
                               uint64_t lba,
                               uint32_t count,
                               const void *buffer);
    void *driver_data;
} sb_block_device_t;

static inline int sb_block_range_valid(const sb_block_device_t *device,
                                       uint64_t lba,
                                       uint32_t count) {
    return device != 0 && device->sector_size != 0u &&
           device->sector_count != 0u && count != 0u &&
           lba < device->sector_count &&
           (uint64_t)count <= device->sector_count - lba;
}

/* Canonical block I/O boundary. 512-byte reads/writes may be satisfied by the
 * shared sector cache; sb_block_flush() establishes persistence to the backing
 * driver for all dirty entries belonging to the device. Filesystems and other
 * callers must not invoke driver callbacks directly. */
sb_block_status_t sb_block_read(sb_block_device_t *device,
                                uint64_t lba,
                                uint32_t count,
                                void *buffer);
sb_block_status_t sb_block_write(sb_block_device_t *device,
                                 uint64_t lba,
                                 uint32_t count,
                                 const void *buffer);
sb_block_status_t sb_block_flush(sb_block_device_t *device);

sb_block_status_t sb_block_register(sb_block_device_t *device);
sb_block_status_t sb_block_unregister(sb_block_device_t *device);
sb_block_device_t *sb_block_get(uint32_t index);
uint32_t sb_block_count(void);
sb_block_status_t sb_block_selftest(void);

#endif
