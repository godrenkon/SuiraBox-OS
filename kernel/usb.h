#ifndef SB_KERNEL_USB_H
#define SB_KERNEL_USB_H

#include <stdint.h>

#define SB_USB_MAX_CONTROLLERS 16u
#define SB_USB_MAX_ENDPOINTS 16u
#define SB_USB_MAX_DESCRIPTOR 256u

typedef enum {
    SB_USB_CONTROLLER_UNKNOWN = 0,
    SB_USB_CONTROLLER_UHCI,
    SB_USB_CONTROLLER_OHCI,
    SB_USB_CONTROLLER_EHCI,
    SB_USB_CONTROLLER_XHCI
} sb_usb_controller_type_t;

typedef enum {
    SB_USB_ENDPOINT_CONTROL = 0,
    SB_USB_ENDPOINT_ISOCHRONOUS,
    SB_USB_ENDPOINT_BULK,
    SB_USB_ENDPOINT_INTERRUPT
} sb_usb_endpoint_type_t;

typedef struct {
    uint8_t address;
    uint8_t attributes;
    uint16_t max_packet_size;
    uint8_t interval;
    sb_usb_endpoint_type_t type;
    uint8_t direction_in;
    uint8_t enabled;
} sb_usb_endpoint_t;

typedef struct {
    uint8_t address;
    uint8_t usb_class;
    uint8_t usb_subclass;
    uint8_t protocol;
    uint8_t configuration_value;
    uint8_t endpoint_count;
    sb_usb_endpoint_t endpoints[SB_USB_MAX_ENDPOINTS];
} sb_usb_device_t;

typedef struct {
    sb_usb_controller_type_t type;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t device_index;
    uint64_t mmio_base;
    uint8_t started;
} sb_usb_controller_t;

typedef int (*sb_usb_transfer_fn)(sb_usb_controller_t *controller, sb_usb_device_t *device,
                                  sb_usb_endpoint_t *endpoint, void *buffer, uint32_t length);

void sb_usb_init(void);
uint32_t sb_usb_controller_count(void);
const sb_usb_controller_t *sb_usb_controller_get(uint32_t index);
int sb_usb_parse_endpoint(const uint8_t *descriptor, uint32_t length, sb_usb_endpoint_t *endpoint);
int sb_usb_parse_device_descriptor(const uint8_t *descriptor, uint32_t length, sb_usb_device_t *device);
int sb_usb_add_endpoint(sb_usb_device_t *device, const sb_usb_endpoint_t *endpoint);

#endif
