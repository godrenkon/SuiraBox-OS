# SuiraBox OS Driver Architecture

## Goal

SuiraBox needs a driver architecture that supports normal desktop hardware while allowing performance-sensitive paths for Minecraft and server workloads.

The driver model keeps hardware-specific code isolated from generic kernel interfaces. A device can be discovered, identified, have resources assigned, bind to a driver, start, stop, suspend/resume, and detach through common infrastructure.

## Layering

```text
Application / Runtime
        |
   Generic subsystem API
        |
   SB Device Core
        |
   Bus / Device Model
        |
   Device Driver
        |
      Hardware
```

Examples of generic subsystem APIs:

- `sb_net` for network devices
- `sb_block` for storage devices
- `sb_gpu` for graphics devices
- `sb_input` for input devices
- `sb_audio` for audio devices

The application must not directly manipulate vendor-specific hardware registers.

## Device model

The initial C ABI is implemented in `kernel/device.h` and `kernel/device.c`.

Each device records:

- device type/class;
- bus type;
- vendor/device identifier;
- revision and IRQ line;
- MMIO/I/O resource ranges;
- lifecycle state;
- bound driver;
- private driver data.

Lifecycle states are:

```text
DISCOVERED
    ↓
IDENTIFIED
    ↓
RESOURCES_ASSIGNED
    ↓
DRIVER_BOUND
    ↓
ACTIVE
    ↓
QUIESCING
    ↓
DRIVER_BOUND
    ↓
DETACHED
```

Probe/start failures enter `FAILED` and are never silently promoted to `ACTIVE`.

Driver callbacks are:

```text
probe(device)
start(device)
stop(device)
remove(device)
suspend(device)
resume(device)
```

Callbacks are optional, but a driver must not claim `ACTIVE` unless `start()` succeeds. The registry bounds the number of live device records and rejects invalid or overflowing resource ranges.

## Bus model

Initial buses:

1. PCI / PCIe
2. platform / firmware-described devices
3. USB later

PCI enumeration now publishes discovered PCI functions into the common device registry. The PCI identity is retained as `driver_data` for later driver binding.

For x86_64 PCs, PCIe is especially important for GPUs, NICs, NVMe controllers, and other high-performance devices.

## DMA and IOMMU

DMA-capable devices must not receive unrestricted access to arbitrary physical memory.

The architecture therefore reserves an interface for:

- DMA buffer allocation;
- DMA mapping/unmapping;
- cache/coherency handling;
- IOMMU integration;
- device memory ownership.

An IOMMU-aware design remains a later security and isolation requirement.

## Interrupts

Drivers do not implement their own global interrupt policy. The kernel owns interrupt routing and exposes a controlled interface to drivers.

## Input foundation

`kernel/input.c` and `kernel/input.h` provide the shared PS/2 polling layer used by the syscall ABI:

- keyboard scancode retrieval;
- three-byte PS/2 mouse packet assembly;
- mouse synchronization on the first packet byte;
- nonblocking reads;
- initialization failure without blocking the boot path.

This keeps raw PS/2 handling out of individual applications and provides a single event source for the GUI and future terminal layer.

## Storage integration

The existing ATA PIO backend registers detected primary-master storage with the generic device registry and publishes its legacy I/O port ranges. Block/VFS remain above the controller implementation.

## First driver milestones

### Stage A — QEMU / virtual devices

- serial output
- basic display/framebuffer
- QEMU block device
- QEMU network device
- timer/interrupt infrastructure

### Stage B — common PC devices

- PCI enumeration and device registration
- NVMe
- Ethernet NIC
- USB input
- ACPI power/device information

### Stage C — graphics

Use a staged graphics path:

```text
UEFI GOP / framebuffer
        |
        v
SB Display Abstraction
        |
        v
Initial GPU driver
        |
        v
Future accelerated graphics
```

Simple framebuffer output remains the bootstrap path; accelerated graphics is a separate capability level.

## Driver isolation

Long-term, drivers should be restartable or isolatable where practical.

Target concept:

```text
Kernel
 |
 +-- Core device management
 |
 +-- Driver service
 |      |
 |      +-- NIC driver
 |      +-- Storage driver
 |      +-- Audio driver
 |      +-- GPU driver
 |
 +-- Security / resource policy
```

The exact user-space versus kernel-space split is still chosen per device class after the corresponding minimal driver exists.

## Minecraft priorities

For Minecraft and Minecraft Server, the most important driver areas are expected to be:

1. GPU / display
2. network
3. NVMe / storage
4. input
5. audio

Implementation order remains dependency- and test-driven.

## Performance rules

SuiraBox does not claim that a custom driver is faster simply because it is custom.

Every optimization requires:

- a defined workload;
- reproducible measurements;
- a baseline;
- before/after results;
- regression checks.
