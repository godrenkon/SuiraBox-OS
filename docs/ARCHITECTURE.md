# SuiraBox OS Architecture

## Project direction

SuiraBox OS (SB) is a general-purpose open-source desktop OS with a Minecraft-first optimization strategy.

The kernel remains general-purpose. Minecraft-specific behavior belongs in policy, runtime, and user-space components rather than hard-coding Minecraft assumptions into the kernel.

## Initial target platform

- Architecture: x86_64
- Initial virtual hardware: QEMU
- Initial firmware/boot environment: to be selected during bootloader implementation
- Build environment: GitHub Actions

Starting with QEMU keeps hardware variables controlled while the kernel is being developed. Physical hardware support will follow after the basic kernel is stable.

## Layer model

```text
+--------------------------------------------------+
| Applications                                     |
| Browser / Terminal / Minecraft / Server / Apps  |
+--------------------------------------------------+
| SB Hub / Desktop / User Services                 |
+--------------------------------------------------+
| Runtime                                          |
| JVM / Native Runtime / Compatibility Layers      |
+--------------------------------------------------+
| SB Kernel                                        |
| Scheduler / Memory / Process / Syscall / VFS    |
| Network / Drivers / Graphics / Security         |
+--------------------------------------------------+
| Hardware                                         |
+--------------------------------------------------+
```

## Kernel subsystems

### CPU and interrupts

- CPU initialization
- Interrupt Descriptor Table
- exception handling
- timer
- interrupt controller support

### Memory

- Physical Memory Manager (PMM)
- Virtual Memory Manager (VMM)
- kernel heap
- user address spaces
- future: buddy allocator, huge pages, NUMA-aware policies

### Scheduling

- preemptive scheduler
- per-thread state
- priorities
- sleep/wakeup
- future: CPU-aware and workload-specific policies

### Processes and threads

- process address spaces
- kernel threads
- user threads
- resource accounting
- isolation

### Syscalls and IPC

- stable syscall boundary
- handles/file descriptors
- pipes
- message queues
- future: shared-memory IPC

### Storage

- VFS abstraction
- block-device layer
- filesystem drivers
- page/cache layer
- asynchronous I/O
- future: snapshots and optimized Minecraft data paths

### Networking

Networking is a first-class subsystem because Minecraft multiplayer, Minecraft servers, browsers, application distribution, and many desktop services depend on it.

Planned layers:

- NIC driver framework
- Ethernet
- IPv4
- IPv6
- TCP
- UDP
- socket API
- packet buffers
- firewall/policy layer
- network monitoring
- future: multi-queue, adaptive interrupt/polling, CPU-aware networking, and validated fast paths

Performance work must be benchmark-driven. The project must not assume that a technique such as zero-copy, polling, CPU pinning, or hardware offload is universally faster.

### Graphics and drivers

The driver architecture will isolate hardware-specific code from generic kernel interfaces.

Initial driver work should target virtual hardware in QEMU. GPU and physical NIC support will be introduced incrementally.

## Minecraft integration

Minecraft integration is primarily a user-space/runtime concern.

Planned components:

```text
SB Hub
  |
  +-- Minecraft Instance Manager
  +-- Minecraft Server Manager
  +-- Runtime Manager
  +-- Mod / Loader Management
  +-- Performance Monitor
  |
  v
SB Runtime / JVM
  |
  v
SB Kernel
```

The kernel provides efficient and safe primitives; the Minecraft-specific layer decides how to use them.

## Performance philosophy

SuiraBox aims to improve practical performance through:

1. low-overhead kernel paths
2. efficient scheduling
3. efficient memory management
4. asynchronous I/O
5. low-overhead networking
6. efficient graphics integration
7. workload-aware policies
8. measurement and benchmarking

No claim of superiority over Linux or Windows should be made without reproducible benchmarks on the same hardware and workload.

## Development principle

Build from a minimal bootable system upward. Every major subsystem should have a small test or observable milestone before the next layer is added.
