# SuiraBox OS Driver Architecture

## Goal

SuiraBox needs a driver architecture that supports normal desktop hardware while allowing performance-sensitive paths for Minecraft and server workloads.

The driver model must keep hardware-specific code isolated from generic kernel interfaces. A device should be discoverable, bindable to a driver, initialized, monitored, and shut down through common infrastructure.

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

Each device should expose metadata such as:

- device type/class
- bus type
- vendor/device identifier
- resources (MMIO, I/O ports, IRQs, DMA)
- power state
- driver state

Drivers should expose lifecycle callbacks conceptually similar to:

```text
probe(device)
start(device)
stop(device)
remove(device)
suspend(device)
resume(device)
```

The exact C ABI is intentionally not frozen yet.

A common device/driver model is a deliberate design choice. Linux uses a unified device model to avoid coupling every device-specific driver to every bus implementation; this is useful prior art, not a requirement to copy Linux's implementation.

## Bus model

Initial buses:

1. PCI / PCIe
2. platform / firmware-described devices
3. USB later

For x86_64 PCs, PCIe is especially important for GPUs, NICs, NVMe controllers, and other high-performance devices.

## DMA and IOMMU

DMA-capable devices must not receive unrestricted access to arbitrary physical memory.

The architecture should therefore reserve an interface for:

- DMA buffer allocation
- DMA mapping/unmapping
- cache/coherency handling
- IOMMU integration
- device memory ownership

An IOMMU-aware design is a future security and isolation requirement, especially for untrusted or hot-pluggable devices.

## Interrupts

Drivers should not each implement their own interrupt policy. The kernel owns interrupt routing and exposes a controlled interface to drivers.

Future network and storage drivers may additionally use polling or batched processing when benchmarks demonstrate a benefit.

## First driver milestones

### Stage A — QEMU / virtual devices

- serial output
- basic display/framebuffer
- QEMU block device
- QEMU network device
- timer/interrupt infrastructure

### Stage B — common PC devices

- PCI enumeration
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

UEFI's Graphics Output Protocol can expose a framebuffer to OS loaders before a high-performance OS graphics driver is active, making it a useful bootstrap display path.

QEMU's VirtIO GPU provides a virtual display/GPU device and supports configurations ranging from simple 2D output to accelerated 3D; however, accelerated guest graphics support introduces additional host/guest requirements. For initial SB development, simple framebuffer output should be preferred over attempting full 3D acceleration immediately.

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

The exact user-space versus kernel-space split will be chosen per device class after the minimal kernel is working. Performance-critical paths may remain kernel-resident initially, while less trusted or less latency-sensitive drivers can later be candidates for stronger isolation.

## Minecraft priorities

For Minecraft and Minecraft Server, the most important driver areas are expected to be:

1. GPU / display
2. network
3. NVMe / storage
4. input
5. audio

But implementation order is based on dependency and testability, not just Minecraft importance.

## Performance rules

Suirabox should not claim that a custom driver is faster simply because it is custom.

Every optimization must have:

- a defined workload
- a reproducible benchmark
- baseline measurements
- before/after results
- regression checks

Potential future optimizations include multi-queue NIC handling, DMA buffer reuse, batched I/O, CPU-aware interrupt routing, and hardware offload.
