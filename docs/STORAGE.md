# SuiraBox Storage Architecture

## Goal

Provide one stable storage interface for ordinary files, applications, Minecraft instances, and server data while keeping device-specific code isolated.

## Layering

```text
Application
    |
SB File API / libc
    |
VFS
    |
SB Block Layer
    |
Storage Driver
    |
Block Device
```

## Block device contract

The first kernel interface is intentionally small:

- sector size
- sector count
- read sectors
- write sectors
- device name

The current prototype uses 512-byte sectors as the baseline interface. Filesystem-specific behavior does not belong in the block layer.

## Device lifecycle

```text
PCI / platform discovery
        |
        v
storage driver probe
        |
        v
block device registration
        |
        v
VFS / filesystem
```

A device driver owns the hardware-specific implementation. Higher layers only depend on the generic block-device interface.

## Development stages

### Stage 1

- block device abstraction
- in-memory/mock device for kernel tests
- QEMU block device discovery

### Stage 2

- read-only filesystem support
- basic file reads
- directory traversal

### Stage 3

- read/write filesystem
- cache
- asynchronous I/O
- error recovery

### Stage 4

- NVMe driver
- storage benchmarking
- Minecraft world/data optimization

## Minecraft data model

Minecraft environments should remain logically isolated:

```text
Minecraft/
├── Instances/
├── Servers/
├── Backups/
└── Cache/
```

Deleting an instance must not implicitly delete unrelated user files. Backups and user data must be separately identifiable.

## Performance policy

Storage optimizations are benchmark-driven. Potential future work includes request batching, asynchronous I/O, cache policy tuning, direct I/O paths, and device-specific optimizations.

No optimization is considered successful without reproducible measurements and regression checks.
