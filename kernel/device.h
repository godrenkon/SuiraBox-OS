#ifndef SB_KERNEL_DEVICE_H
#define SB_KERNEL_DEVICE_H

#include <stdint.h>

#define SB_DEVICE_NAME_MAX 31u
#define SB_DEVICE_MAX 128u

typedef enum {
    SB_DEVICE_BUS_PLATFORM = 0,
    SB_DEVICE_BUS_PCI = 1,
    SB_DEVICE_BUS_USB = 2
} sb_device_bus_t;

typedef enum {
    SB_DEVICE_CLASS_UNKNOWN = 0,
    SB_DEVICE_CLASS_STORAGE,
    SB_DEVICE_CLASS_NETWORK,
    SB_DEVICE_CLASS_DISPLAY,
    SB_DEVICE_CLASS_INPUT,
    SB_DEVICE_CLASS_AUDIO,
    SB_DEVICE_CLASS_POWER,
    SB_DEVICE_CLASS_BRIDGE,
    SB_DEVICE_CLASS_USB_HOST,
    SB_DEVICE_CLASS_OTHER
} sb_device_class_t;

typedef enum {
    SB_DEVICE_DISCOVERED = 0,
    SB_DEVICE_IDENTIFIED,
    SB_DEVICE_RESOURCES_ASSIGNED,
    SB_DEVICE_DRIVER_BOUND,
    SB_DEVICE_ACTIVE,
    SB_DEVICE_QUIESCING,
    SB_DEVICE_DETACHED,
    SB_DEVICE_FAILED
} sb_device_state_t;

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t flags;
} sb_device_resource_t;

typedef struct sb_device sb_device_t;

typedef struct {
    int (*probe)(sb_device_t *device);
    int (*start)(sb_device_t *device);
    int (*stop)(sb_device_t *device);
    int (*remove)(sb_device_t *device);
    int (*suspend)(sb_device_t *device);
    int (*resume)(sb_device_t *device);
} sb_device_driver_t;

struct sb_device {
    uint32_t index;
    sb_device_bus_t bus;
    sb_device_class_t class_id;
    sb_device_state_t state;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint8_t programming_interface;
    uint8_t irq_line;
    uint8_t resource_count;
    char name[SB_DEVICE_NAME_MAX + 1u];
    sb_device_resource_t resources[6];
    const sb_device_driver_t *driver;
    void *driver_data;
};

void sb_device_init(void);
sb_device_t *sb_device_register(sb_device_bus_t bus, sb_device_class_t class_id,
                                uint16_t vendor_id, uint16_t device_id,
                                const char *name);
int sb_device_set_resource(sb_device_t *device, uint8_t slot,
                           uint64_t base, uint64_t size, uint32_t flags);
int sb_device_bind(sb_device_t *device, const sb_device_driver_t *driver);
int sb_device_start(sb_device_t *device);
int sb_device_stop(sb_device_t *device);
int sb_device_remove(sb_device_t *device);
int sb_device_suspend(sb_device_t *device);
int sb_device_resume(sb_device_t *device);
uint32_t sb_device_count(void);
sb_device_t *sb_device_get(uint32_t index);

#endif
