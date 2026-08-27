# SuiraBox Filesystem Strategy

## Goal

SuiraBox should support ordinary desktop files, application data, Minecraft instances, Minecraft servers, and backups without tying the VFS to one filesystem format.

## Layering

```text
Application / Runtime
        |
     File API
        |
       VFS
        |
+-------+-------+
|               |
FAT32          Future filesystems
|               |
+-------+-------+
        |
 SB Block Layer
        |
 Storage Driver
        |
 Device
```

## Initial implementation

The first on-disk filesystem target is FAT32 read-only support. It is intentionally limited:

- validate the boot sector
- read FAT metadata
- follow cluster chains
- enumerate the root directory
- read regular files
- support 8.3 names initially
- do not modify the filesystem

Long filenames, subdirectories, write support, timestamps, permissions, journaling, and advanced caching are later milestones.

## Why read-only first

A read-only filesystem gives the kernel a smaller and safer milestone. It allows boot-time and CI tests to prove that data can be located and read from an actual block device before write paths are introduced.

## Data model

The user-facing namespace should remain filesystem-independent:

```text
/
├── System/
├── Users/
├── Apps/
├── Minecraft/
│   ├── Instances/
│   ├── Servers/
│   ├── Backups/
│   └── Cache/
└── Runtime/
```

## Future write support

When write support is introduced, updates should use safe ordering and atomic replacement where possible. User data and cache data must remain distinguishable, and deleting an application or Minecraft instance must not implicitly delete unrelated user data.

## Performance direction

The VFS should be independent from performance policy. Later work may add:

- page cache
- readahead
- async I/O
- direct I/O
- request batching
- filesystem-specific tuning

Every optimization must be benchmarked against a baseline.
