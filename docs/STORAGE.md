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
- optional device cache flush
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

## Durability boundary and current proof

`sb_block_flush(device)` writes dirty software-cache entries before invoking
the driver's `flush` callback. A writeback failure skips the device barrier and
keeps the dirty entry retryable. A device-barrier failure propagates to VFS;
retry must invoke the barrier even when software entries are already clean.
Unregister retains the device on either failure. A null flush callback declares
that the backend has no volatile write cache requiring an explicit barrier.
ATA PIO provides the `CACHE FLUSH` command as its device barrier.

`FILE_SYNC` follows FILE handle -> VFS file -> FAT32 adapter -> mount sync ->
block writeback -> device barrier. FAT32 supports overwriting and extending
existing files, including allocating a first cluster for an empty file. Extension
flushes data before mirrored FATs, then publishes directory size; FILE_SYNC is
required for final directory durability. FILE_CREATE publishes an empty 8.3
entry without allocation; its first write uses this same extension path.

GitHub Actions opts in with `make STORAGE_DURABILITY_PROOF=1`. The fixture is a
64 MiB FAT32 image with one additional sector outside the BPB volume boundary.
The kernel checks exact geometry, boot signature and every byte of the seed
before staging a dirty sector. After userspace FILE_SYNC and QEMU exit, the host
verifies every byte of that sector from the reopened image. Normal builds omit
the staging path and userspace write probe; changing the build mode rebuilds the kernel and userspace through a mode
stamp. The image and QEMU log are retained as CI artifacts.

The CI userspace probe also overwrites and appends 768 bytes to `/disk/RUNTIME.TXT`, syncs, closes and
reopens it. After QEMU exit, mtools extracts the file for exact content/size
comparison and verifies a separate guard file; `fsck.fat -n` checks filesystem
consistency. Host tests exercise fragmented/nested files, sector/cluster
boundaries, read-only permissions, malformed chains and short I/O progress.
`host-fat32-extend-test` adds allocation, backend-observed metadata ordering,
FSInfo invalidation, FAT rollback and failed-rollback write quarantine.
The creation host test exercises root/nested/deleted slots, duplicate protection,
directory/node-cache exhaustion and ordered end-marker publication. QEMU creates
NEWFILE.TXT, writes and syncs it, then the host compares its exact contents.

`make host-storage-durability-test` uses separate volatile and durable device
buffers to detect a missing or incorrectly ordered barrier. It exercises
writeback failure, barrier failure, retries with a clean software cache, and
failed unregister. The image checker rejects unchanged seeds and partial writes.
The QEMU image check proves the integrated write/flush path with a file backend;
it does not simulate physical power loss or prove filesystem atomicity. Those
require crash/recovery transactions and power-loss tests in a later stage.
