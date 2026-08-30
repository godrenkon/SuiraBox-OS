#include <assert.h>
#include "../kernel/device.h"
#include "../kernel/gpu.h"

int main(void) {
    sb_device_init();
    sb_device_t *pci_display = sb_device_register(SB_DEVICE_BUS_PCI, SB_DEVICE_CLASS_DISPLAY, 0x1234u, 0x5678u, "gpu");
    sb_device_t *framebuffer = sb_device_register(SB_DEVICE_BUS_PLATFORM, SB_DEVICE_CLASS_DISPLAY, 0u, 0u, "framebuffer");
    assert(pci_display != 0 && framebuffer != 0);
    pci_display->class_code = 0x03u;
    pci_display->subclass = 0x00u;
    sb_gpu_init();
    assert(sb_gpu_count() == 2u);
    assert(sb_gpu_get(0u)->level == SB_GPU_DETECTED);
    assert(sb_gpu_get(0u)->framebuffer_fallback == 0u);
    assert(sb_gpu_get(1u)->level == SB_GPU_BASIC_FALLBACK);
    assert(sb_gpu_get(1u)->framebuffer_fallback == 1u);
    assert(sb_gpu_level_name(SB_GPU_ACCELERATED)[0] == 'A');
    assert(sb_gpu_get(2u) == 0);
    return 0;
}
