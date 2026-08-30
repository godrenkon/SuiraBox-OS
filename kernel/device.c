#include "device.h"

static sb_device_t devices[SB_DEVICE_MAX];
static uint32_t device_count_value;

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0u;
    if (dst == 0) return;
    if (src != 0) {
        while (i < SB_DEVICE_NAME_MAX && src[i] != '\0') {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

static int state_allows_start(sb_device_state_t state) {
    return state == SB_DEVICE_DRIVER_BOUND || state == SB_DEVICE_IDENTIFIED || state == SB_DEVICE_RESOURCES_ASSIGNED;
}

void sb_device_init(void) {
    for (uint32_t i = 0u; i < SB_DEVICE_MAX; ++i) {
        devices[i] = (sb_device_t){0};
        devices[i].state = SB_DEVICE_DETACHED;
    }
    device_count_value = 0u;
}

sb_device_t *sb_device_register(sb_device_bus_t bus, sb_device_class_t class_id,
                                uint16_t vendor_id, uint16_t device_id,
                                const char *name) {
    if (device_count_value >= SB_DEVICE_MAX) return 0;
    sb_device_t *device = &devices[device_count_value];
    *device = (sb_device_t){0};
    device->index = device_count_value;
    device->bus = bus;
    device->class_id = class_id;
    device->state = SB_DEVICE_DISCOVERED;
    device->vendor_id = vendor_id;
    device->device_id = device_id;
    copy_name(device->name, name);
    ++device_count_value;
    return device;
}

int sb_device_set_resource(sb_device_t *device, uint8_t slot,
                           uint64_t base, uint64_t size, uint32_t flags) {
    if (device == 0 || slot >= 6u || device->state == SB_DEVICE_DETACHED) return -1;
    if (size != SB_DEVICE_RESOURCE_SIZE_UNKNOWN && base > UINT64_MAX - size) return -1;
    device->resources[slot] = (sb_device_resource_t){ .base = base, .size = size, .flags = flags };
    if (slot >= device->resource_count) device->resource_count = (uint8_t)(slot + 1u);
    if (device->state == SB_DEVICE_DISCOVERED || device->state == SB_DEVICE_IDENTIFIED)
        device->state = SB_DEVICE_RESOURCES_ASSIGNED;
    return 0;
}

int sb_device_bind(sb_device_t *device, const sb_device_driver_t *driver) {
    if (device == 0 || driver == 0 || device->state == SB_DEVICE_DETACHED || device->state == SB_DEVICE_FAILED) return -1;
    if (driver->probe != 0 && driver->probe(device) != 0) { device->state = SB_DEVICE_FAILED; return -1; }
    device->driver = driver;
    device->state = SB_DEVICE_DRIVER_BOUND;
    return 0;
}

int sb_device_start(sb_device_t *device) {
    if (device == 0 || device->driver == 0 || device->state == SB_DEVICE_ACTIVE || !state_allows_start(device->state)) return -1;
    if (device->driver->start != 0 && device->driver->start(device) != 0) { device->state = SB_DEVICE_FAILED; return -1; }
    device->state = SB_DEVICE_ACTIVE;
    return 0;
}

int sb_device_stop(sb_device_t *device) {
    if (device == 0 || device->driver == 0 || device->state != SB_DEVICE_ACTIVE) return -1;
    device->state = SB_DEVICE_QUIESCING;
    if (device->driver->stop != 0 && device->driver->stop(device) != 0) { device->state = SB_DEVICE_FAILED; return -1; }
    device->state = SB_DEVICE_DRIVER_BOUND;
    return 0;
}

int sb_device_remove(sb_device_t *device) {
    if (device == 0 || device->state == SB_DEVICE_DETACHED) return -1;
    if (device->driver != 0 && device->driver->remove != 0 && device->driver->remove(device) != 0) { device->state = SB_DEVICE_FAILED; return -1; }
    device->driver = 0;
    device->driver_data = 0;
    device->state = SB_DEVICE_DETACHED;
    return 0;
}

int sb_device_suspend(sb_device_t *device) {
    if (device == 0 || device->driver == 0 || device->state != SB_DEVICE_ACTIVE) return -1;
    if (device->driver->suspend != 0 && device->driver->suspend(device) != 0) return -1;
    device->state = SB_DEVICE_QUIESCING;
    return 0;
}

int sb_device_resume(sb_device_t *device) {
    if (device == 0 || device->driver == 0 || device->state != SB_DEVICE_QUIESCING) return -1;
    if (device->driver->resume != 0 && device->driver->resume(device) != 0) return -1;
    device->state = SB_DEVICE_ACTIVE;
    return 0;
}

uint32_t sb_device_count(void) { return device_count_value; }
sb_device_t *sb_device_get(uint32_t index) { return index < device_count_value ? &devices[index] : 0; }
