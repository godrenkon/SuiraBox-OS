#include "pci.h"
#include "device.h"

#define PCI_BAR_COUNT 6u
#define PCI_BAR_IO_MASK 0x3u
#define PCI_BAR_MEM_MASK 0xFull
#define PCI_HEADER_TYPE_OFFSET 0x0Cu
#define PCI_BAR_OFFSET 0x10u
#define PCI_STATUS_OFFSET 0x04u
#define PCI_STATUS_CAP_LIST 0x10u
#define PCI_CAP_PTR_OFFSET 0x34u
#define PCI_CAP_ID_MASK 0xFFu
#define PCI_CAP_PTR_MASK 0xFCu
#define PCI_CAPABILITY_LIMIT 48u

static inline void outl(uint16_t port, uint32_t value) { __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port)); }
static inline uint32_t inl(uint16_t port) { uint32_t value; __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port)); return value; }
static uint32_t pci_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)device << 11) |
           ((uint32_t)function << 8) | ((uint32_t)(offset & 0xFCu));
}
uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    if (device >= PCI_MAX_DEVICES || function >= PCI_MAX_FUNCTIONS) return 0xFFFFFFFFu;
    outl(PCI_CONFIG_ADDRESS, pci_config_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}
uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    const uint32_t value = pci_config_read32(bus, device, function, offset & 0xFCu);
    return (uint16_t)((value >> ((offset & 2u) * 8u)) & 0xFFFFu);
}
static uint16_t pci_status(uint8_t bus, uint8_t device, uint8_t function) { return (uint16_t)(pci_config_read32(bus, device, function, PCI_STATUS_OFFSET) >> 16); }
static uint8_t pci_cap_ptr(uint8_t bus, uint8_t device, uint8_t function) { return (uint8_t)(pci_config_read32(bus, device, function, PCI_CAP_PTR_OFFSET) & 0xFFu); }

int pci_find_capability(uint8_t bus, uint8_t device, uint8_t function, uint8_t capability_id, uint8_t *offset_out) {
    if (offset_out == 0 || device >= PCI_MAX_DEVICES || function >= PCI_MAX_FUNCTIONS ||
        (pci_status(bus, device, function) & PCI_STATUS_CAP_LIST) == 0u) return -1;
    uint8_t pointer = (uint8_t)(pci_cap_ptr(bus, device, function) & PCI_CAP_PTR_MASK);
    for (uint32_t count = 0u; pointer >= 0x40u && pointer < 0xFCu && count < PCI_CAPABILITY_LIMIT; ++count) {
        const uint32_t header = pci_config_read32(bus, device, function, pointer);
        const uint8_t id = (uint8_t)(header & PCI_CAP_ID_MASK);
        if (id == capability_id) { *offset_out = pointer; return 0; }
        const uint8_t next = (uint8_t)((header >> 8) & PCI_CAP_PTR_MASK);
        if (id == 0u || next == pointer) break;
        pointer = next;
    }
    return -1;
}
static uint8_t pci_class(uint8_t bus, uint8_t device, uint8_t function) { return (uint8_t)(pci_config_read32(bus, device, function, 0x08u) >> 24); }
static uint8_t pci_subclass(uint8_t bus, uint8_t device, uint8_t function) { return (uint8_t)(pci_config_read32(bus, device, function, 0x08u) >> 16); }
static uint8_t pci_prog_if(uint8_t bus, uint8_t device, uint8_t function) { return (uint8_t)(pci_config_read32(bus, device, function, 0x08u) >> 8); }
static uint8_t pci_header_type(uint8_t bus, uint8_t device, uint8_t function) { return (uint8_t)(pci_config_read32(bus, device, function, PCI_HEADER_TYPE_OFFSET) >> 16); }
static sb_device_class_t pci_device_class(uint8_t class_code, uint8_t subclass) {
    switch (class_code) {
        case PCI_CLASS_MASS_STORAGE: return SB_DEVICE_CLASS_STORAGE;
        case PCI_CLASS_NETWORK: return SB_DEVICE_CLASS_NETWORK;
        case PCI_CLASS_DISPLAY: return SB_DEVICE_CLASS_DISPLAY;
        case PCI_CLASS_MULTIMEDIA: return SB_DEVICE_CLASS_AUDIO;
        case 0x0Cu: return subclass == 0x03u ? SB_DEVICE_CLASS_USB_HOST : SB_DEVICE_CLASS_OTHER;
        case PCI_CLASS_BRIDGE: return SB_DEVICE_CLASS_BRIDGE;
        default: return SB_DEVICE_CLASS_OTHER;
    }
}
static const char *pci_class_name(uint8_t class_code, uint8_t subclass) {
    if (class_code == 0x0Cu && subclass == 0x03u) return "usb-host";
    switch (class_code) {
        case PCI_CLASS_MASS_STORAGE: return "mass-storage";
        case PCI_CLASS_NETWORK: return "network";
        case PCI_CLASS_DISPLAY: return "display";
        case PCI_CLASS_MULTIMEDIA: return "multimedia";
        case PCI_CLASS_BRIDGE: return "bridge";
        default: return "other";
    }
}
static void serial_char(char c) {
    while (1) { uint8_t status; __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD)); if (status & 0x20u) break; }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}
static void serial_write(const char *s) { while (s != 0 && *s != '\0') serial_char(*s++); }
static void serial_hex16(uint16_t value) { static const char digits[] = "0123456789ABCDEF"; for (int shift = 12; shift >= 0; shift -= 4) serial_char(digits[(value >> shift) & 0xFu]); }
static void serial_hex8(uint8_t value) { static const char digits[] = "0123456789ABCDEF"; serial_char(digits[(value >> 4) & 0xFu]); serial_char(digits[value & 0xFu]); }
static void pci_register_bars(sb_device_t *device, uint8_t bus, uint8_t slot, uint8_t function) {
    if (device == 0 || (pci_header_type(bus, slot, function) & 0x7Fu) != 0u) return;
    uint8_t resource_slot = 0u;
    for (uint8_t bar = 0u; bar < PCI_BAR_COUNT && resource_slot < SB_DEVICE_MAX_RESOURCES; ++bar) {
        const uint8_t offset = (uint8_t)(PCI_BAR_OFFSET + bar * 4u);
        const uint32_t low = pci_config_read32(bus, slot, function, offset);
        if (low == 0u || low == 0xFFFFFFFFu) continue;
        const uint32_t flags = (low & 0x1u) != 0u ? 0x1u : 0x2u;
        uint64_t base = (uint64_t)(low & ((low & 0x1u) != 0u ? ~PCI_BAR_IO_MASK : ~PCI_BAR_MEM_MASK));
        if ((low & 0x1u) == 0u && ((low >> 1) & 0x3u) == 0x2u && bar + 1u < PCI_BAR_COUNT) {
            const uint32_t high = pci_config_read32(bus, slot, function, (uint8_t)(offset + 4u));
            base |= (uint64_t)high << 32;
            ++bar;
        }
        if (sb_device_set_resource(device, resource_slot, base, SB_DEVICE_RESOURCE_SIZE_UNKNOWN, flags) == 0) ++resource_slot;
    }
}
static void pci_register_capabilities(sb_device_t *device, uint8_t bus, uint8_t slot, uint8_t function) {
    if (device == 0 || (pci_status(bus, slot, function) & PCI_STATUS_CAP_LIST) == 0u) return;
    uint8_t pointer = (uint8_t)(pci_cap_ptr(bus, slot, function) & PCI_CAP_PTR_MASK);
    device->first_capability = pointer;
    for (uint32_t count = 0u; pointer >= 0x40u && pointer < 0xFCu && count < PCI_CAPABILITY_LIMIT; ++count) {
        const uint32_t header = pci_config_read32(bus, slot, function, pointer);
        const uint8_t id = (uint8_t)(header & PCI_CAP_ID_MASK);
        const uint8_t next = (uint8_t)((header >> 8) & PCI_CAP_PTR_MASK);
        if (device->capability_count < SB_DEVICE_MAX_CAPABILITIES) device->capabilities[device->capability_count++] = id;
        if (id == 0u || next == pointer) break;
        pointer = next;
    }
}
void pci_enumerate(void) {
    serial_write("PCI: scanning bus/device/function space...\r\n");
    unsigned int found = 0u;
    for (uint16_t bus = 0u; bus < PCI_MAX_BUSES && found < 128u; ++bus) {
        for (uint8_t device = 0u; device < PCI_MAX_DEVICES && found < 128u; ++device) {
            for (uint8_t function = 0u; function < PCI_MAX_FUNCTIONS && found < 128u; ++function) {
                const uint16_t vendor = pci_config_read16((uint8_t)bus, device, function, 0x00u);
                if (vendor == 0xFFFFu) continue;
                const uint16_t id = pci_config_read16((uint8_t)bus, device, function, 0x02u);
                const uint8_t class_code = pci_class((uint8_t)bus, device, function);
                const uint8_t subclass = pci_subclass((uint8_t)bus, device, function);
                const uint8_t prog_if = pci_prog_if((uint8_t)bus, device, function);
                const uint8_t header_type = pci_header_type((uint8_t)bus, device, function);
                sb_device_t *registered = sb_device_register(SB_DEVICE_BUS_PCI, pci_device_class(class_code, subclass), vendor, id, pci_class_name(class_code, subclass));
                if (registered != 0) {
                    registered->state = SB_DEVICE_IDENTIFIED;
                    registered->bus_number = (uint8_t)bus;
                    registered->device_number = device;
                    registered->function_number = function;
                    registered->header_type = header_type;
                    registered->class_code = class_code;
                    registered->subclass = subclass;
                    registered->revision = (uint8_t)pci_config_read32((uint8_t)bus, device, function, 0x08u);
                    registered->programming_interface = prog_if;
                    registered->irq_line = (uint8_t)(pci_config_read32((uint8_t)bus, device, function, 0x3Cu) & 0xFFu);
                    registered->driver_data = (void *)(uintptr_t)(((uint64_t)bus << 16) | ((uint64_t)device << 8) | function);
                    pci_register_bars(registered, (uint8_t)bus, device, function);
                    pci_register_capabilities(registered, (uint8_t)bus, device, function);
                }
                serial_char('0'); serial_char('0'); serial_char(':'); serial_hex8((uint8_t)bus);
                serial_char('.'); serial_hex8(device); serial_char('.'); serial_char('0' + function);
                serial_write(" vendor=0x"); serial_hex16(vendor); serial_write(" device=0x"); serial_hex16(id);
                serial_write(" class=0x"); serial_hex8(class_code); serial_write("/0x"); serial_hex8(subclass);
                serial_write(" prog=0x"); serial_hex8(prog_if); serial_write(" ("); serial_write(pci_class_name(class_code, subclass)); serial_write(")\r\n");
                ++found;
                if (function == 0u && (header_type & 0x80u) == 0u) break;
            }
        }
    }
    serial_write("PCI: scan complete.\r\n");
}
