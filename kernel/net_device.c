#include "net_device.h"

static sb_net_device_t net_devices[SB_NET_MAX_DEVICES];
static uint32_t net_device_count_value;

void sb_net_device_init(void) {
    net_device_count_value = 0u;
    for (uint32_t i = 0u; i < SB_NET_MAX_DEVICES; ++i)
        net_devices[i] = (sb_net_device_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && net_device_count_value < SB_NET_MAX_DEVICES; ++i) {
        const sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_id != SB_DEVICE_CLASS_NETWORK) continue;
        net_devices[net_device_count_value] = (sb_net_device_t){
            .device_index = device->index,
            .state = SB_NET_DISCOVERED,
            .vendor_id = device->vendor_id,
            .device_id = device->device_id,
            .mac = {0u, 0u, 0u, 0u, 0u, 0u}
        };
        ++net_device_count_value;
    }
}

uint32_t sb_net_device_count(void) { return net_device_count_value; }

const sb_net_device_t *sb_net_device_get(uint32_t index) {
    return index < net_device_count_value ? &net_devices[index] : 0;
}
