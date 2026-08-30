#include <assert.h>
#include <stdint.h>
#include "../kernel/device.h"

static int probes;
static int starts;
static int stops;
static int removes;
static int suspends;
static int resumes;
static int fail_start;
static int fail_probe;
static int fail_stop;
static int fail_remove;

static int probe(sb_device_t *device) { assert(device != 0); ++probes; return fail_probe ? -1 : 0; }
static int start(sb_device_t *device) { assert(device != 0); ++starts; return fail_start ? -1 : 0; }
static int stop(sb_device_t *device) { assert(device != 0); ++stops; return fail_stop ? -1 : 0; }
static int remove_device(sb_device_t *device) { assert(device != 0); ++removes; return fail_remove ? -1 : 0; }
static int suspend_device(sb_device_t *device) { assert(device != 0); ++suspends; return 0; }
static int resume_device(sb_device_t *device) { assert(device != 0); ++resumes; return 0; }

int main(void) {
    static const sb_device_driver_t driver = {
        .probe = probe,
        .start = start,
        .stop = stop,
        .remove = remove_device,
        .suspend = suspend_device,
        .resume = resume_device
    };
    static const char maximum_name[] = "1234567890123456789012345678901";

    sb_device_init();
    assert(sb_device_count() == 0u);
    sb_device_t *device = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_NETWORK, 0x1234u, 0x5678u, maximum_name);
    assert(device != 0 && sb_device_count() == 1u);
    device->bus_number = 2u;
    device->device_number = 7u;
    device->function_number = 3u;
    assert(sb_device_find_pci(2u, 7u, 3u) == device);
    assert(sb_device_find_pci(2u, 7u, 4u) == 0);
    assert(device->state == SB_DEVICE_DISCOVERED);
    assert(sizeof(device->name) == SB_DEVICE_NAME_MAX + 1u);
    assert(device->name[SB_DEVICE_NAME_MAX] == '\0');
    assert(sb_device_set_resource(device, 0u, 0x1000u, 0x100u, 3u) == 0);
    assert(device->state == SB_DEVICE_RESOURCES_ASSIGNED);
    assert(sb_device_bind(device, &driver) == 0 && probes == 1);
    assert(device->state == SB_DEVICE_DRIVER_BOUND);
    assert(sb_device_start(device) == 0 && starts == 1 && device->state == SB_DEVICE_ACTIVE);
    assert(sb_device_suspend(device) == 0 && suspends == 1 && device->state == SB_DEVICE_QUIESCING);
    assert(sb_device_resume(device) == 0 && resumes == 1 && device->state == SB_DEVICE_ACTIVE);
    assert(sb_device_stop(device) == 0 && stops == 1 && device->state == SB_DEVICE_DRIVER_BOUND);
    assert(sb_device_remove(device) == 0 && removes == 1 && device->state == SB_DEVICE_DETACHED);
    assert(sb_device_start(device) != 0);
    assert(sb_device_suspend(device) != 0);
    assert(sb_device_resume(device) != 0);
    assert(sb_device_set_resource(device, 0u, UINT64_MAX - 1u, 4u, 0u) != 0);

    sb_device_t *failed = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_DISPLAY, 0u, 0u, 0);
    assert(failed != 0);
    fail_start = 1;
    assert(sb_device_bind(failed, &driver) == 0);
    assert(sb_device_start(failed) != 0);
    assert(failed->state == SB_DEVICE_FAILED);
    assert(sb_device_start(failed) != 0);
    assert(sb_device_remove(failed) == 0);

    sb_device_t *probe_failed = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_INPUT, 0u, 0u, "probe-fail");
    assert(probe_failed != 0);
    fail_start = 0;
    fail_probe = 1;
    assert(sb_device_bind(probe_failed, &driver) != 0);
    assert(probe_failed->state == SB_DEVICE_FAILED);
    assert(probe_failed->driver == 0);
    assert(sb_device_remove(probe_failed) == 0);

    sb_device_t *stop_failed = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_AUDIO, 0u, 0u, "stop-fail");
    assert(stop_failed != 0);
    fail_probe = 0;
    assert(sb_device_bind(stop_failed, &driver) == 0);
    assert(sb_device_start(stop_failed) == 0);
    fail_stop = 1;
    assert(sb_device_stop(stop_failed) != 0);
    assert(stop_failed->state == SB_DEVICE_FAILED);
    fail_stop = 0;
    assert(sb_device_remove(stop_failed) == 0);

    sb_device_t *remove_failed = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_STORAGE, 0u, 0u, "remove-fail");
    assert(remove_failed != 0);
    assert(sb_device_bind(remove_failed, &driver) == 0);
    fail_remove = 1;
    assert(sb_device_remove(remove_failed) != 0);
    assert(remove_failed->state == SB_DEVICE_FAILED);
    fail_remove = 0;
    assert(sb_device_remove(remove_failed) == 0);
    return 0;
}
