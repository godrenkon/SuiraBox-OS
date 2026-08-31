#include <stdint.h>
#include "../kernel/net_device.h"

uint32_t sb_net_device_count(void) {
    return 0u;
}

const sb_net_device_t *sb_net_device_get(uint32_t index) {
    (void)index;
    return 0;
}

int sb_net_device_send(uint32_t index, const uint8_t *frame, uint16_t length) {
    (void)index;
    (void)frame;
    (void)length;
    return -1;
}

int sb_net_device_receive(uint32_t index, uint8_t *frame, uint16_t capacity, uint16_t *length) {
    (void)index;
    (void)frame;
    (void)capacity;
    (void)length;
    return -1;
}
