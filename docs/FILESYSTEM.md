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

The current on-disk filesystem supports FAT32 reads and fixed-size overwrites:

- validate the boot sector
- read FAT metadata
- follow cluster chains
- enumerate the root directory
- read regular files
- support 8.3 names initially
- traverse 8.3 subdirectories
- overwrite existing regular files within their current size on writable devices
- preserve FAT, directory metadata, file size and bytes outside the write range

Long filenames, allocation, file extension/creation, timestamps, general permissions, journaling, and advanced caching are later milestones. FAT read-only attributes and read-only block devices reject write access.

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

The current overwrite path validates the complete size-implied cluster chain
before mutation. It requires a bounded chain ending in EOC at the expected last
cluster, rejects cycles/premature termination/out-of-range clusters, and rejects
the root directory cluster as file data. Partial sectors use read-modify-write.
A failure after accepted progress returns a short write; a failure before
progress returns I/O error. Requests crossing EOF fail before any write.
The file size, FAT chain and directory entry never change in this stage.

FILE_WRITE accepts data into the software sector cache. FILE_SYNC writes back
dirty data and then invokes the device cache barrier. No metadata ordering is
needed for these size-preserving operations. Multi-sector overwrite is not
atomic under I/O failure or power loss. Allocation/extension will need explicit
data-before-FAT-before-directory ordering and crash/recovery tests.

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
