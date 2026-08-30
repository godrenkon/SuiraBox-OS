#ifndef SB_KERNEL_PCI_H
#define SB_KERNEL_PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu
#define PCI_MAX_BUSES      256u
#define PCI_MAX_DEVICES    32u
#define PCI_MAX_FUNCTIONS  8u

#define PCI_CLASS_UNCLASSIFIED 0x00u
#define PCI_CLASS_MASS_STORAGE 0x01u
#define PCI_CLASS_NETWORK      0x02u
#define PCI_CLASS_DISPLAY      0x03u
#define PCI_CLASS_MULTIMEDIA   0x04u
#define PCI_CLASS_BRIDGE       0x06u

#define PCI_COMMAND_IO_SPACE      0x0001u
#define PCI_COMMAND_MEMORY_SPACE  0x0002u
#define PCI_COMMAND_BUS_MASTER    0x0004u

#define PCI_CAP_ID_MSI  0x05u
#define PCI_CAP_ID_MSIX 0x11u
#define PCI_CAP_ID_PM   0x01u

uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
int pci_find_capability(uint8_t bus, uint8_t device, uint8_t function, uint8_t capability_id, uint8_t *offset_out);
void pci_enumerate(void);

#endif /* SB_KERNEL_PCI_H */
