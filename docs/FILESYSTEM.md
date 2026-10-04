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

The current on-disk filesystem supports FAT32 reads, overwrites and file extension:

- validate the boot sector
- read FAT metadata
- follow cluster chains
- enumerate the root directory
- read regular files
- support 8.3 names initially
- traverse 8.3 subdirectories
- overwrite and extend existing regular files on writable devices
- allocate and zero up to eight new clusters per extension request
- update mirrored FATs and the physical directory slot with ordered flushes

Long filenames, file creation, timestamps, general permissions, journaling, and advanced caching are later milestones. FAT read-only attributes and read-only block devices reject write access. Reads honor the BPB active FAT; extension currently requires mirrored FATs.

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

## Write and extension contract

The current overwrite path validates the complete size-implied cluster chain
before mutation. It requires a bounded chain ending in EOC at the expected last
cluster, rejects cycles/premature termination/out-of-range clusters, and rejects
the root directory cluster as file data. Partial sectors use read-modify-write.
A failure after accepted progress returns a short write; a failure before
progress returns I/O error. Size-preserving overwrites leave FAT and directory
metadata unchanged. Seeking beyond EOF is rejected, so extension cannot create holes.

FILE_WRITE accepts data into the software sector cache. FILE_SYNC writes back
dirty data and then invokes the device cache barrier. No metadata ordering is
needed for these size-preserving operations. Multi-sector overwrite is not
atomic under I/O failure or power loss.

Extension first validates the physical directory entry, the existing chain and
free clusters in every FAT copy. Insufficient space, 32-bit size overflow and
more than eight new clusters reject before data changes. Newly allocated clusters
are zeroed, including slack bytes. The complete requested data is written and
flushed before FAT links are changed. FAT changes preserve reserved high nibbles,
invalidate valid primary FSInfo hints, and are flushed before the directory's
first cluster and size are accepted into the cache. FILE_SYNC makes this final
directory publication durable. Node identity uses the physical directory slot,
so existing readers and repeated lookups share the new size.

An extension failure reports IO with zero accepted bytes and unchanged logical
size/offset. Previously existing bytes overlapped by that request may already
have changed; extension is not an atomic overwrite. Before directory acceptance,
metadata failures trigger FAT rollback and a flush. Failed rollback quarantines
further writes and file sync on this mount; recovery requires filesystem repair
and remount rather than a false success. A failed FILE_SYNC after successful
extension retains the grown logical file and can be retried normally.

Power loss between the FAT and directory phases can leave orphaned allocation.
There is no journal or crash-recovery transaction yet. The current syscall path
serializes filesystem operations; concurrent filesystem writers will need locks.

`make host-fat32-extend-test` checks fragmented allocation, empty/nested files,
data/FAT/directory ordering at backend writes, failure/rollback/quarantine,
shared node identity, slack zeroing, mirror consistency and FSInfo invalidation.
CI appends across a cluster boundary in QEMU, reopens the file, and checks exact
contents, guard-file preservation and `fsck.fat -n` after QEMU exits.

The metadata rules follow the [Microsoft FAT32 specification](https://www.cs.fsu.edu/~cop4610t/assignments/project3/spec/fatspec.pdf).

Future creation and replacement should retain this ordering and add a recovery policy. User data and cache data must remain distinguishable, and deleting an application or Minecraft instance must not implicitly delete unrelated user data.

## Performance direction

The VFS should be independent from performance policy. Later work may add:

- page cache
- readahead
- async I/O
- direct I/O
- request batching
- filesystem-specific tuning

Every optimization must be benchmarked against a baseline.
