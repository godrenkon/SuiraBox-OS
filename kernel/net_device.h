#ifndef SB_KERNEL_NET_DEVICE_H
#define SB_KERNEL_NET_DEVICE_H

#include <stdint.h>
#include "device.h"

#define SB_NET_MAX_DEVICES 16u

typedef enum {
    SB_NET_DOWN = 0,
    SB_NET_DISCOVERED,
    SB_NET_READY,
    SB_NET_ERROR
} sb_net_state_t;

typedef enum {
    SB_NET_CONTROLLER_UNKNOWN = 0,
    SB_NET_CONTROLLER_ETHERNET,
    SB_NET_CONTROLLER_WIFI,
    SB_NET_CONTROLLER_OTHER
} sb_net_controller_type_t;

typedef struct {
    uint32_t device_index;
    sb_net_state_t state;
    sb_net_controller_type_t controller_type;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t programming_interface;
    uint8_t mac[6];
} sb_net_device_t;

void sb_net_device_init(void);
uint32_t sb_net_device_count(void);
const sb_net_device_t *sb_net_device_get(uint32_t index);

#endif
