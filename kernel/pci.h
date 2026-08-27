#ifndef SB_KERNEL_PCI_H
#define SB_KERNEL_PCI_H

#include <stdint.h>

/* PCI Configuration Mechanism #1 uses these x86 I/O ports. */
#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu

#define PCI_MAX_BUSES      256u
#define PCI_MAX_DEVICES    32u
#define PCI_MAX_FUNCTIONS  8u

/* Class codes used by the initial human-readable inventory. */
#define PCI_CLASS_UNCLASSIFIED 0x00u
#define PCI_CLASS_MASS_STORAGE 0x01u
#define PCI_CLASS_NETWORK      0x02u
#define PCI_CLASS_DISPLAY      0x03u
#define PCI_CLASS_MULTIMEDIA   0x04u
#define PCI_CLASS_BRIDGE       0x06u

uint32_t pci_config_read32(uint8_t bus, uint8_t device,
                           uint8_t function, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t device,
                           uint8_t function, uint8_t offset);

void pci_enumerate(void);

#endif /* SB_KERNEL_PCI_H */
