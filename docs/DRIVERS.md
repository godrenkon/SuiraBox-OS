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
- PCI class/subclass/programming interface when applicable;
- revision and IRQ line;
- MMIO/I/O resource ranges;
- PCI standard capability identifiers;
- lifecycle state;
- bound driver;
- private driver data.

Unknown resource sizes are represented explicitly as `SB_DEVICE_RESOURCE_SIZE_UNKNOWN`; discovery never writes PCI BAR probe values merely to determine size.

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

## PCI foundation

`kernel/pci.c` enumerates PCI configuration mechanism #1 bus/device/function space and publishes each discovered function to the common device registry.

The PCI layer records:

- vendor/device ID;
- class/subclass/programming interface;
- revision;
- interrupt line;
- standard BAR base addresses;
- standard capability IDs.

`pci_find_capability()` provides bounded capability lookup for later MSI/MSI-X/power-management drivers without changing device state during discovery.

## Input foundation

`kernel/input.c` and `kernel/input.h` provide the shared PS/2 polling layer used by the syscall ABI:

- keyboard scancode retrieval;
- three-byte PS/2 mouse packet assembly;
- mouse first-byte synchronization;
- nonblocking reads;
- initialization failure without blocking boot;
- ordered kernel input-event queue with sequence numbers.

The PS/2 controller is published as a platform input device with its I/O ports recorded as resources. This keeps raw hardware handling out of applications and gives GUI and future terminal code one event source.

## Storage integration

The existing ATA PIO backend registers detected primary-master storage with the generic device registry and publishes its legacy I/O port ranges. Block/VFS remain above the controller implementation.

`kernel/nvme.c` discovers PCI NVMe controllers and records their MMIO BAR without dereferencing device memory during early boot. Controller-register probing is explicitly deferred until the corresponding MMIO mapping/driver stage exists, preventing unsafe pre-VMM hardware access.

## USB foundation

PCI USB host controllers are classified by programming interface as UHCI, OHCI, EHCI, or xHCI when identified. Controller records retain the first suitable MMIO resource.

The common USB layer validates standard USB device and endpoint descriptors and records endpoint type, direction, maximum packet size, and polling interval.

`kernel/usb_transfer.c` adds a bounded controller-agnostic FIFO transfer scheduler with ordered sequence numbers, active/completed/failed/cancelled states, and completion callbacks. `kernel/usb_class.c` adds bounded class-driver registration and wildcard class/subclass/protocol matching. Controller-specific rings, DMA, transaction processing, and hotplug remain below this abstraction.

## Network foundation

PCI network devices are published through `kernel/net_device.c`. Ethernet and wireless-class devices are distinguished from other network controllers, while MAC/address state remains unset until a real NIC driver reads it from hardware.

The IP/TCP/UDP protocol stack is a later subsystem; this layer is hardware discovery only.

## Audio foundation

PCI audio devices are published through `kernel/audio.c`. High Definition Audio and AC'97 classes are distinguished from other multimedia controllers. Codec enumeration and PCM playback/capture remain driver-level work.

## GPU / display compatibility

`kernel/gpu.c` defines the explicit compatibility ladder:

```text
DETECTED
BASIC_FALLBACK
DRIVER_PRESENT
FUNCTIONAL
ACCELERATED
PERFORMANCE_VERIFIED
HARDWARE_VERIFIED
```

A platform framebuffer is represented as `BASIC_FALLBACK`; a PCI display device begins at `DETECTED`. No acceleration or performance level is claimed without an actual driver and corresponding validation.

## ACPI / power foundation

`kernel/acpi.c` validates Multiboot-provided RSDP data, locates a valid XSDT/RSDT and FADT, and extracts the DSDT and PM1 power-control information when valid. `_S5_` sleep types are accepted only after AML/table bounds and checksums have passed.

`kernel/power.c` exposes reboot/shutdown capabilities and prefers validated ACPI S5 shutdown before emulator/legacy development fallbacks. Power actions are not advertised as ACPI-native unless the required firmware data has been validated.

## Bus model

Initial buses:

1. PCI / PCIe
2. platform / firmware-described devices
3. USB host-controller abstraction

For x86_64 PCs, PCIe is especially important for GPUs, NICs, NVMe controllers, USB controllers, and other high-performance devices.

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

## First driver milestones

### Stage A — QEMU / virtual devices

- serial output
- basic display/framebuffer
- QEMU block device
- QEMU network device
- timer/interrupt infrastructure

### Stage B — common PC devices

- PCI enumeration and device registration
- NVMe discovery foundation
- Ethernet/Wi-Fi discovery foundation
- USB host-controller discovery foundation
- USB descriptor/endpoint validation
- USB transfer scheduler and class-driver registry foundation
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
