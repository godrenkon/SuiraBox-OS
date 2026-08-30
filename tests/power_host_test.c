#include <assert.h>
#include "../kernel/power.h"
#include "../kernel/device.h"

static sb_device_t test_power_device;

sb_device_t *sb_device_register(sb_device_bus_t bus, sb_device_class_t class_id,
                                uint16_t vendor_id, uint16_t device_id,
                                const char *name) {
    (void)bus;
    (void)vendor_id;
    (void)device_id;
    (void)name;
    test_power_device = (sb_device_t){
        .index = 0u,
        .class_id = class_id,
        .state = SB_DEVICE_DISCOVERED
    };
    return &test_power_device;
}

int main(void) {
    sb_power_init();
    assert(sb_power_capabilities() == (SB_POWER_REBOOT | SB_POWER_SHUTDOWN));
    return 0;
}
