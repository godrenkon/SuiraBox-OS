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

The current on-disk filesystem supports FAT32 reads, exclusive creation, overwrites and file extension:

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
- create empty regular files in existing directory slots, then allocate on write

Long filenames, timestamps, general permissions, journaling, and advanced caching are later milestones. FAT read-only attributes and read-only block devices reject write access. Reads honor the BPB active FAT; creation and extension currently require mirrored FATs.

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

## Exclusive regular-file creation

FILE_CREATE (ABI 39) creates an empty file and returns a FILE handle. WRITE is
required; READ is optional. The final name accepts ASCII letters, digits, `_`
and `-` in a base of 1..8 characters and an optional extension of 1..3. Names
are stored in uppercase. Invalid names are rejected rather than shortened.
Duplicate 8.3 aliases, including aliases with LFN entries, return EXISTS without
truncation. Parent directories must already exist and permit CREATE.

The generic VFS CREATE capability is separate from file WRITE. Namespace
creation uses the normal mount resolver for the parent, protects mounted roots,
and rejects trailing slash or terminal dot components. The syscall copies the
whole path and reserves heap/handle space before invoking the backend. The FAT32
backend validates the directory chain and reserves node-cache capacity before
publishing an empty short entry. It reuses a deleted slot or the end marker;
a full directory grows by one cluster when allocation space is available.

When moving the end marker within a sector, the new entry and successor marker
are accepted together. Across sectors or fragmented clusters, the successor
marker is flushed first. This keeps garbage after the old end invisible. Slots
immediately following live orphaned LFN entries are not reused. Initial creation
does not allocate file-data clusters. A full parent may allocate a directory
cluster as described below. Subsequent writes use the extension contract;
FILE_SYNC persists the created entry and data, and failures can be retried.

`make host-fat32-create-test` covers root/nested creation, deleted slots, aliases,
invalid paths, read-only/quarantined mounts, full directories/node caches,
fragmented end-marker ordering and injected I/O/barrier failures. The opt-in
QEMU probe checks invalid pointers/access and exhausted handle capacity before
creation, duplicate rejection, writing, syncing and reopening NEWFILE.TXT.
mtools compares that file after QEMU exits and fsck checks the resulting image.

## Directory growth during creation

When every existing slot is occupied, FILE_CREATE appends one cluster to the
parent directory chain. Both root and nested directories support this path.
Existing chain links are validated against every mirrored FAT copy, and the
selected free cluster must be free in every copy. A node-cache slot is reserved
before writes. Lack of space/cache capacity returns LIMIT before publication;
an orphaned live LFN at the tail also prevents adding a short entry after it.

The entire new cluster is initialized with an empty short entry followed by
zeroed end-marker/slack bytes. A data flush makes it durable before the new FAT
EOC and old tail link are changed. The FAT copies and invalidated primary FSInfo
hint are then flushed. Only after that succeeds does creation return its FILE
handle. Existing directory entries and first-cluster identities stay unchanged;
directory sizes remain zero in FAT entries. File writes still require FILE_SYNC.

Before the FAT phase, failed writes/barriers leave only unreferenced free-cluster
contents. A FAT error invokes the existing rollback and flush contract. If that
rollback succeeds, the old directory chain remains authoritative and creation
can be retried without a duplicate entry. Failed rollback quarantines further
mutations; reachability is then uncertain until repair/remount. Power loss can
still interrupt FAT-copy updates, so this ordering is not an atomic transaction.

`make host-fat32-directory-growth-test` uses separate volatile/durable buffers
to prove initialized data is durable before FAT links, and tests root/nested
growth, mirror inconsistency, space exhaustion, I/O/barrier failures, rollback
and quarantine. CI fills the original root cluster with 14 empty filler files,
then creates NEWFILE.TXT through userspace. The reopened image checker requires
a matching new link in both FATs, the new entry in the added cluster, zeroed
slack, and byte-for-byte preservation of the original cluster except RUNTIME's
expected size update. Corrupted mirrors, existing entries and slack fail the
checker. The original baseline is retained as an artifact alongside the image.

## Mount-time recovery status

Before enabling writable FAT32 nodes, mount reads FAT[1]'s clean-shutdown bit
(`0x08000000`) and no-hard-error bit (`0x04000000`). These meanings follow the
[Microsoft FAT specification, page 18](https://www.cs.fsu.edu/~cop4610t/assignments/project3/spec/fatspec.pdf).
When mirroring is enabled, every FAT copy is inspected. Any clear status bit or
status-bit disagreement sets recovery flags and mounts the filesystem read-only.
Reserved high nibbles and other FAT[1] bits are not compared. When mirroring is
disabled, only BPB's selected active FAT supplies status; inactive copies are
ignored. An unreadable required status sector rejects the mount rather than
assuming the volume is clean.

Recovery-required volumes retain lookup, directory enumeration, existing-file
reads and read-handle FILE_SYNC. Writable file handles, regular-file writes,
FILE_CREATE and DIRECTORY_CREATE return ACCESS/RIGHTS, including direct backend
calls and cached nested nodes. Mount and sync do not clear or repair FAT[1], and
kernel serial output reports the read-only mode and reason flags (unclean=1,
hard-error=2, mirrored-status disagreement=4). The block device may still be
writable; this is a filesystem policy, not a device-wide write lock.

This is a recovery entry condition, not a complete integrity check or automatic
repair. It does not scan chains for lost/cross-linked clusters and does not
implement transactions. SuiraBox now persists dirty status before its first
mutation in a mount session, but does not yet implement clean unmount. A clean
marker is not an integrity proof and ordered marking does not make interrupted
writes atomic. Clean shutdown, repair and journal recovery remain future work.

`make host-fat32-recovery-test` covers both status bits, mirrored disagreement,
reserved-nibble differences, active-FAT selection, status-read failures, clean
writability, nested reads, mutation denial and preservation of the entire image.
CI separately builds `STORAGE_RECOVERY_PROOF=1` (durability proof disabled), boots
writable QEMU disks with dirty, hard-error and inconsistent-status fixtures, and
requires existing reads/sync/enumeration followed by WRITE-open/FILE_CREATE/mkdir
rejection. SHA-256 comparison after each boot requires every image byte to remain
unchanged, including recovery evidence. Normal-build fixture removal is checked.

## Dirty marker before mutation

Before accepting the first regular-file overwrite/extension, FILE_CREATE or
DIRECTORY_CREATE mutation, `sb_fat32_begin_write()` clears FAT[1]'s clean-shutdown
bit in every mirrored FAT (or only the selected active FAT). It preserves all
other bits and bytes, writes through the canonical cache, and completes the
device barrier before file data, allocation or directory publication can begin.
The mounted session remains writable after this successful barrier; subsequent
mutations reuse its durable marker. Invalid paths, duplicate names, zero-length
writes and preflight resource failures do not initiate marking.

A marker read/write/barrier error returns IO with zero file-write progress and
quarantines further mutation. A partial marker may already be cached or durable;
it is never restored to clean. No data-sector mutation starts after a failed
marker. Reserved inode/handle slots are released on failed creation. Metadata
ordering, rollback and later data-sync failures retain their existing contracts.

FILE_SYNC, CLOSE and successful mkdir leave the dirty bit clear. There is no
clean-unmount protocol yet, so any volume mutated by SuiraBox mounts read-only on
the next boot until external offline repair establishes a clean state. This
conservative intermediate behavior is intentional; sync durability is distinct
from clean shutdown. Prior hard-error bits are preserved; persisting new hard-error
evidence remains future work. Raw block-device writes bypass this filesystem policy.

`make host-fat32-dirty-test` uses volatile/durable buffers to prove the marker
barrier precedes actual data/metadata writeback, tests overwrite/extension/create/
mkdir/active-FAT paths, one marker per session, marker faults, inode cleanup,
zero/rejected requests and cache-loss/remount read-only behavior. Existing fault
tests explicitly prepare a dirty session and reset their baseline/counters before
testing later data/FAT/publication phases. CI's opt-in STORAGE_DIRTY_PROOF creates
PENDING.TXT without sync and stops at a checkpoint. After QEMU is terminated,
whole-image hashing requires exactly the mirrored dirty-bit changes and no
published PENDING entry. Rebooting that same disk with the recovery proof must
deny mutations and preserve every image byte. This is a software-cache-loss test
with a file backend, not a complete physical power-loss model or journal proof.

The normal durability image remains dirty. CI repairs only a disposable image
copy with offline fsck, checks it again and compares nested file content; the
original image and its recovery evidence remain preserved as artifacts.

## Exclusive directory creation

DIRECTORY_CREATE (ABI 40) returns a READ|QUERY directory handle. VFS providers
advertise a separate MKDIR capability and borrowed-node callback, so read-only
providers and directories cannot create children. The namespace shares file
creation's parent resolution, path normalization, duplicate protection and mount
root protection. Parent directories must already exist; there is no mkdir-p mode.

The FAT32 backend preflights the name, duplicate aliases, parent chain, mirrored
FATs and node-cache capacity. A full parent requires two free clusters (child and
parent extension) before writes begin. It allocates one child cluster, initializes `.` to
the child and `..` to its parent (zero for a root parent), and zeros every remaining
byte of every sector. Child data is flushed before its mirrored FAT EOC and
invalidated primary FSInfo hints are flushed. Only then is the parent's short
directory entry published using the existing slot/end-marker/growth machinery.
The parent entry has attribute DIRECTORY and size zero, and is flushed before
returning. Dot entries remain hidden from lookup/iteration; lexical path handling
continues in the namespace. Created directories accept nested directories/files.

Before parent publication, failed I/O frees the child allocation with a rollback
barrier. A failed parent-growth rollback instead retains the child allocation,
because reachability may already be uncertain. Once the parent entry is accepted,
a failed publication flush quarantines the mount rather than freeing referenced
storage. The syscall returns IO with no handle; the directory may then be visible,
and repair/remount is required before further writes. This is an explicit error
contract, not a promise that every failed mkdir leaves no name. There is no delete
or on-disk journal yet, so power loss can still leave orphaned allocations or
partially updated FAT copies.

`make host-fat32-mkdir-test` checks multi-sector initialization, root/nested dot
parents, iteration, nested file persistence after remount, full-parent growth,
duplicate/path/read-only/no-space rejection, injected data/FAT/FSInfo/barrier
failures, rollback, and publication quarantine. QEMU checks reserved flags,
invalid pointers, handle exhaustion, missing/read-only parents and duplicate
creation, then creates SAVES/WORLDS/LEVEL.DAT and verifies enumeration and reopened
contents. After exit, mtools and the image checker validate exact content, mirrored
EOCs, non-overlapping allocations, dot parents and zeroed directory slack; fsck
checks the resulting image. Corruption tests exercise the image checker itself.

Creation is not a power-loss transaction. Atomic replacement and recovery
remain future work. User data and cache data must remain distinguishable, and
deleting an application or Minecraft instance must not implicitly delete
unrelated user data.

## Performance direction

The VFS should be independent from performance policy. Later work may add:

- page cache
- readahead
- async I/O
- direct I/O
- request batching
- filesystem-specific tuning

Every optimization must be benchmarked against a baseline.
