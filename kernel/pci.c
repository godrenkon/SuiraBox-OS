#include "pci.h"

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t pci_config_address(uint8_t bus, uint8_t device,
                                   uint8_t function, uint8_t offset) {
    return 0x80000000u |
           ((uint32_t)bus << 16) |
           ((uint32_t)device << 11) |
           ((uint32_t)function << 8) |
           ((uint32_t)(offset & 0xFCu));
}

uint32_t pci_config_read32(uint8_t bus, uint8_t device,
                           uint8_t function, uint8_t offset) {
    if (device >= PCI_MAX_DEVICES || function >= PCI_MAX_FUNCTIONS) {
        return 0xFFFFFFFFu;
    }

    outl(PCI_CONFIG_ADDRESS, pci_config_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t device,
                           uint8_t function, uint8_t offset) {
    const uint32_t value = pci_config_read32(bus, device, function, offset & 0xFCu);
    const uint8_t shift = (uint8_t)((offset & 2u) * 8u);
    return (uint16_t)((value >> shift) & 0xFFFFu);
}

static uint8_t pci_class(uint8_t bus, uint8_t device, uint8_t function) {
    const uint32_t value = pci_config_read32(bus, device, function, 0x08u);
    return (uint8_t)(value >> 24);
}

static uint8_t pci_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    const uint32_t value = pci_config_read32(bus, device, function, 0x08u);
    return (uint8_t)(value >> 16);
}

static uint8_t pci_prog_if(uint8_t bus, uint8_t device, uint8_t function) {
    const uint32_t value = pci_config_read32(bus, device, function, 0x08u);
    return (uint8_t)(value >> 8);
}

static const char *pci_class_name(uint8_t class_code) {
    switch (class_code) {
        case PCI_CLASS_MASS_STORAGE: return "mass-storage";
        case PCI_CLASS_NETWORK:      return "network";
        case PCI_CLASS_DISPLAY:      return "display";
        case PCI_CLASS_MULTIMEDIA:   return "multimedia";
        case PCI_CLASS_BRIDGE:       return "bridge";
        default:                     return "other";
    }
}

static void serial_char(char c) {
    while (1) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20u) {
            break;
        }
    }
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

static void serial_write(const char *s) {
    while (*s != '\0') {
        serial_char(*s++);
    }
}

static void serial_hex16(uint16_t value) {
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4) {
        serial_char(digits[(value >> shift) & 0xFu]);
    }
}

static void serial_hex8(uint8_t value) {
    static const char digits[] = "0123456789ABCDEF";
    serial_char(digits[(value >> 4) & 0xFu]);
    serial_char(digits[value & 0xFu]);
}

void pci_enumerate(void) {
    serial_write("PCI: scanning bus/device/function space...\r\n");

    unsigned int found = 0;
    for (uint16_t bus = 0; bus < PCI_MAX_BUSES; ++bus) {
        for (uint8_t device = 0; device < PCI_MAX_DEVICES; ++device) {
            for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; ++function) {
                const uint16_t vendor = pci_config_read16((uint8_t)bus, device, function, 0x00u);
                if (vendor == 0xFFFFu) {
                    continue;
                }

                const uint16_t id = pci_config_read16((uint8_t)bus, device, function, 0x02u);
                const uint8_t class_code = pci_class((uint8_t)bus, device, function);
                const uint8_t subclass = pci_subclass((uint8_t)bus, device, function);
                const uint8_t prog_if = pci_prog_if((uint8_t)bus, device, function);

                serial_char('0');
                serial_char('0');
                serial_char(':');
                serial_hex8((uint8_t)bus);
                serial_char('.');
                serial_hex8(device);
                serial_char('.');
                serial_char('0' + function);
                serial_write(" vendor=0x");
                serial_hex16(vendor);
                serial_write(" device=0x");
                serial_hex16(id);
                serial_write(" class=0x");
                serial_hex8(class_code);
                serial_write("/0x");
                serial_hex8(subclass);
                serial_write(" prog=0x");
                serial_hex8(prog_if);
                serial_write(" (");
                serial_write(pci_class_name(class_code));
                serial_write(")\r\n");

                ++found;
                if (found >= 128u) {
                    serial_write("PCI: inventory limit reached (128 devices).\r\n");
                    serial_write("PCI: scan complete.\r\n");
                    return;
                }

                /* Avoid probing unused functions of a non-multifunction device. */
                if (function == 0u) {
                    const uint8_t header_type =
                        (uint8_t)(pci_config_read32((uint8_t)bus, device, function, 0x0Cu) >> 16);
                    if ((header_type & 0x80u) == 0u) {
                        break;
                    }
                }
            }
        }
    }

    serial_write("PCI: scan complete.\r\n");
}
