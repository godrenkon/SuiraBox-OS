#include "audio.h"
#include "device.h"

static sb_audio_device_t audio_devices[SB_AUDIO_MAX_DEVICES];
static uint32_t audio_device_count_value;

static sb_audio_controller_type_t controller_type(const sb_device_t *device) {
    if (device == 0 || device->class_code != 0x04u) return SB_AUDIO_CONTROLLER_UNKNOWN;
    if (device->subclass == 0x03u) return SB_AUDIO_CONTROLLER_HDA;
    if (device->subclass == 0x01u) return SB_AUDIO_CONTROLLER_AC97;
    return SB_AUDIO_CONTROLLER_OTHER;
}

void sb_audio_init(void) {
    audio_device_count_value = 0u;
    for (uint32_t i = 0u; i < SB_AUDIO_MAX_DEVICES; ++i) audio_devices[i] = (sb_audio_device_t){0};
    for (uint32_t i = 0u; i < sb_device_count() && audio_device_count_value < SB_AUDIO_MAX_DEVICES; ++i) {
        const sb_device_t *device = sb_device_get(i);
        if (device == 0 || device->class_id != SB_DEVICE_CLASS_AUDIO) continue;
        audio_devices[audio_device_count_value] = (sb_audio_device_t){
            .device_index = device->index,
            .state = SB_AUDIO_DISCOVERED,
            .controller_type = controller_type(device),
            .vendor_id = device->vendor_id,
            .device_id = device->device_id,
            .class_code = device->class_code,
            .subclass = device->subclass,
            .programming_interface = device->programming_interface
        };
        ++audio_device_count_value;
    }
}

uint32_t sb_audio_device_count(void) { return audio_device_count_value; }
const sb_audio_device_t *sb_audio_device_get(uint32_t index) {
    return index < audio_device_count_value ? &audio_devices[index] : 0;
}
