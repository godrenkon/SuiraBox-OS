# SuiraBox OS PCI / PCIe Design

## Goal

PCI/PCIe is the first hardware discovery layer for modern x86_64 PCs. The design must let SB enumerate devices and later bind them to isolated, reusable drivers.

## Discovery path

```text
CPU / Firmware
      |
      v
PCI Root Complex
      |
      v
PCI Enumerator
      |
      +-- Bus
      +-- Device
      +-- Function
      |
      v
SB Device Core
      |
      v
Driver Matching
      |
      v
Device Driver
```

## Device identity

Each PCI function should expose at least:

- vendor ID
- device ID
- class code
- subclass / programming interface
- bus / device / function address (BDF)
- BAR resources
- interrupt capabilities
- command / status information

## Resource model

The generic device layer should represent:

- MMIO regions
- I/O port regions where applicable
- IRQ / MSI / MSI-X resources
- DMA capabilities
- bus mastering state

Drivers request resources from the device core instead of directly making global assumptions.

## Initial implementation

The first implementation will be read-only discovery. It should enumerate the PCI hierarchy and print devices through the serial console.

No device driver binding is required for the first milestone.

## Safety

PCI configuration accesses must be validated so malformed device information cannot cause arbitrary memory access. BAR mappings must be checked before MMIO access is permitted.

## Future

PCIe support will become the discovery foundation for:

- NVMe storage
- Ethernet NICs
- GPUs
- USB controllers
- audio controllers
- other expansion devices

Advanced capabilities such as MSI-X, ACS, SR-IOV, power management, and IOMMU integration are later milestones and must not block the initial enumerator.
