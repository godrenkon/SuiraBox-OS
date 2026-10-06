# SuiraBox Syscall ABI

## Version

Current userspace ABI version: **1**.

Canonical numeric definitions live in `include/suirabox/syscall_abi.h`. Handle metadata, object types, and rights live in `include/suirabox/handle_abi.h`. Kernel and userspace compile against these shared public headers; syscall numbers and exposed handle constants must not be duplicated as private magic values.

## x86_64 entry contract

SuiraBox currently enters the kernel through IDT vector `0x80` using `int $0x80`.

| Register | Meaning |
| --- | --- |
| `rax` | syscall number on entry; result/error on return |
| `rdi` | argument 0 |
| `rsi` | argument 1 |
| `rdx` | argument 2 |
| `r10` | argument 3 |
| `r8` | argument 4 |

The entry stub saves the complete GPR state in the same `sb_irq_frame_t` representation used by interrupt-driven scheduling. A blocking syscall can therefore select another task's saved frame, and the blocked task can later resume from its exact saved syscall frame before `iretq`.

`rflags`, `rip`, user `rsp`, and user `ss` are restored by `iretq`. ABI version 1 only promises the documented input registers and the result in `rax`; preservation of the complete frame is a kernel implementation property.

## Return convention

`rax` is interpreted as a signed 64-bit result:

- non-negative: success/result
- negative: stable ABI error

Current errors:

| Value | Name | Meaning |
| ---: | --- | --- |
| `-1` | `SB_SYS_ERROR_INVALID` | invalid syscall/argument/state |
| `-2` | `SB_SYS_ERROR_FAULT` | userspace pointer is unmapped or lacks required access |
| `-3` | `SB_SYS_ERROR_LIMIT` | request/table exceeds a supported limit |
| `-4` | `SB_SYS_ERROR_STALE` | opaque handle generation is no longer valid |
| `-5` | `SB_SYS_ERROR_RIGHTS` | handle lacks required rights |
| `-6` | `SB_SYS_ERROR_NOT_FOUND` | named object/path or directory entry does not exist |
| `-7` | `SB_SYS_ERROR_IO` | underlying object/provider I/O failed |
| `-8` | `SB_SYS_ERROR_WOULD_BLOCK` | nonblocking operation cannot make progress yet |
| `-9` | `SB_SYS_ERROR_CLOSED` | peer/end of an IPC object is closed |
| `-10` | `SB_SYS_ERROR_TIMEOUT` | timed blocking wait reached its deadline |
| `-11` | `SB_SYS_ERROR_EXISTS` | named object already exists |
| `-12` | `SB_SYS_ERROR_BUSY` | active service client or volume references/nested mounts |

## Version 1 syscall table

The current public maximum syscall number is **45**.

| Number | Name | Arguments | Result |
| ---: | --- | --- | --- |
| 0 | `SB_SYS_GET_TICKS` | none | monotonic PIT tick count |
| 1 | `SB_SYS_PROCESS_ID` | none | current PID |
| 2 | `SB_SYS_EXIT` | `rdi=exit_code` | does not return on success |
| 3 | `SB_SYS_SLEEP` | `rdi=delay_ticks` | 0 after wake |
| 4 | `SB_SYS_SPAWN` | `rdi=image_selector` | child PID; legacy bootstrap path |
| 5 | `SB_SYS_WAIT_PROCESS` | `rdi=child_pid` | child exit code after wait |
| 6 | `SB_SYS_ABI_VERSION` | none | ABI version |
| 7 | `SB_SYS_LOG_WRITE` | `rdi=user_buffer`, `rsi=length` | bytes written |
| 8 | `SB_SYS_ABI_INFO` | `rdi=writable sb_syscall_abi_info_t*` | 0 and fills ABI info |
| 9 | `SB_SYS_PROCESS_OPEN_SELF` | none | PROCESS handle with QUERY |
| 10 | `SB_SYS_HANDLE_INFO` | `rdi=handle`, `rsi=writable sb_handle_info_t*` | 0 and fills type/rights |
| 11 | `SB_SYS_HANDLE_CLOSE` | `rdi=handle` | 0; invalidates handle generation |
| 12 | `SB_SYS_SPAWN_REQUEST` | `rdi=readable sb_spawn_request_t*` | dynamically allocated child PID |
| 13 | `SB_SYS_FILE_OPEN_BOOT_MODULE` | `rdi=name`, `rsi=name_length` | compatibility read-only FILE handle |
| 14 | `SB_SYS_FILE_READ` | `rdi=file`, `rsi=writable buffer`, `rdx=length` | bytes read |
| 15 | `SB_SYS_FILE_SEEK` | `rdi=file`, `rsi=absolute_offset` | 0 |
| 16 | `SB_SYS_FILE_OPEN` | `rdi=absolute path`, `rsi=path_length`, `rdx=access` | FILE handle |
| 17 | `SB_SYS_DIRECTORY_OPEN` | `rdi=absolute path`, `rsi=path_length` | DIRECTORY handle |
| 18 | `SB_SYS_DIRECTORY_READ` | `rdi=directory`, `rsi=writable sb_directory_entry_t*` | 0 or `NOT_FOUND` at EOF |
| 19 | `SB_SYS_PIPE_CREATE` | `rdi=writable sb_pipe_handles_t*` | 0 and fills read/write PIPE handles |
| 20 | `SB_SYS_PIPE_READ` | `rdi=read_pipe`, `rsi=writable buffer`, `rdx=length` | bytes read, 0 at EOF, or `WOULD_BLOCK` |
| 21 | `SB_SYS_PIPE_WRITE` | `rdi=write_pipe`, `rsi=readable buffer`, `rdx=length` | bytes written, `WOULD_BLOCK`, or `CLOSED` |
| 22 | `SB_SYS_EVENT_CREATE` | `rdi=initial_signal_state` | EVENT handle |
| 23 | `SB_SYS_EVENT_WAIT` | `rdi=event`, `rsi=timeout_ticks` | 0, `WOULD_BLOCK`, or `TIMEOUT` |
| 24 | `SB_SYS_EVENT_SIGNAL` | `rdi=event` | 0 |
| 25 | `SB_SYS_EVENT_RESET` | `rdi=event` | 0 |
| 26 | `SB_SYS_THREAD_CREATE` | `rdi=user_entry` | new thread ID |
| 27 | `SB_SYS_MESSAGE_QUEUE_CREATE` | none | MESSAGE_QUEUE handle |
| 28 | `SB_SYS_MESSAGE_QUEUE_SEND` | `rdi=queue`, `rsi=readable buffer`, `rdx=length` | bytes sent |
| 29 | `SB_SYS_MESSAGE_QUEUE_RECEIVE` | `rdi=queue`, `rsi=writable buffer`, `rdx=capacity` | bytes received |
| 30 | `SB_SYS_SHARED_MEMORY_CREATE` | `rdi=size_bytes` | SHARED_MEMORY handle |
| 31 | `SB_SYS_SHARED_MEMORY_MAP` | `rdi=memory`, `rsi=user_address`, `rdx=access` | mapped user address |
| 32 | `SB_SYS_SHARED_MEMORY_UNMAP` | `rdi=user_address` | 0 |
| 33 | `SB_SYS_SERVICE_REGISTER` | `rdi=name`, `rsi=name_length` | server SERVICE handle |
| 34 | `SB_SYS_SERVICE_CONNECT` | `rdi=name`, `rsi=name_length` | client SERVICE handle |
| 35 | `SB_SYS_SERVICE_SEND` | `rdi=service`, `rsi=readable buffer`, `rdx=length` | bytes sent |
| 36 | `SB_SYS_SERVICE_RECEIVE` | `rdi=service`, `rsi=writable buffer`, `rdx=capacity` | bytes received |
| 37 | `SB_SYS_FILE_SYNC` | `rdi=file` | 0 after backing-store/device flush |
| 38 | `SB_SYS_FILE_WRITE` | `rdi=file`, `rsi=readable buffer`, `rdx=length` | bytes accepted; may be short |
| 39 | `SB_SYS_FILE_CREATE` | `rdi=absolute path`, `rsi=path length`, `rdx=READ/WRITE access` | new FILE handle; exclusive creation |
| 40 | `SB_SYS_DIRECTORY_CREATE` | `rdi=absolute path`, `rsi=path length`, `rdx=0` | new DIRECTORY handle; exclusive durable creation |
| 41 | `SB_SYS_VOLUME_UNMOUNT` | `rdi=exact mount path`, `rsi=path length`, `rdx=0` | 0 after finalization and namespace detach; PID 1 only |
| 42 | `SB_SYS_FILE_RENAME` | `rdi=source path`, `rsi=source length`, `rdx=destination path`, `r10=destination length`, `r8=0` | 0 after exclusive same-directory rename and publication flush |
| 43 | `SB_SYS_DISPLAY_INFO` | `rdi=info output`, `rsi=32`, `rdx=0` | 0 after copying display metadata; NOT_FOUND when no mapped display exists |
| 44 | `SB_SYS_DISPLAY_PRESENT` | `rdi=present request`, `rsi=32`, `rdx=0` | 0 after bounded RGB rectangle presentation; PID 1 only |
| 45 | `SB_SYS_KEY_EVENT_READ` | `rdi=writable sb_key_event_t*`, `rsi=32`, `rdx=0` | 0 after event copy; WOULD_BLOCK if empty; PID 1 only |

Syscall 4 and syscall 13 are retained for ABI-v1 compatibility. New code should prefer the versioned spawn request and generic VFS path interfaces.

## ABI info routing

The original frame dispatcher owns the historic 0..16 implementation table. Object-specific calls 17 and above are append-only extensions routed by the syscall entry layer. This split is internal only: userspace sees one ABI and `SB_SYS_ABI_INFO` reports the public maximum, 45.

## Keyboard event transport

KEY_EVENT_READ uses the exact 32-byte structure from `input_abi.h`:
`u32 size`, `u16 version/type`, `u16 keycode/flags`, `u32 modifiers`,
`u64 sequence`, `u32 dropped/reserved`. Version is 1; reserved is zero.
KEY events carry a normalized physical US-labelled position and DOWN/REPEAT
flags, not a scancode or encoded character. Modifier bits describe the state
after that event. OVERFLOW events have zero key/flags and a snapshot of the
cumulative saturating discarded-entry count. See [keyboard input](KEYBOARD_INPUT.md)
for key values, decoding and resynchronization policy.

Init/PID 1 owns the boot input queue; children receive RIGHTS before pointer
access. Null output, wrong size or nonzero flags return INVALID. An unavailable
keyboard returns NOT_FOUND. Validate the complete writable output even if the
queue is empty; invalid mappings return FAULT, and a valid empty queue returns
WOULD_BLOCK. Copy before pop so a failed copy preserves the head event. The
current UP interrupt gates serialize IRQ and syscall queue access; no blocking
wait, per-process routing, input handles or SMP synchronization is provided yet.

## Userspace pointer rules

A ring3 pointer is never trusted from its numeric range alone. For every copy the kernel:

1. rejects overflow and addresses outside the user range;
2. walks the target process page tables;
3. requires `PRESENT|USER` at every level;
4. requires effective `WRITABLE` for kernel-to-user copies;
5. validates every page touched;
6. copies through resolved physical mappings rather than directly dereferencing an untrusted VA.

The QEMU smoke proves copy-in from `.rodata`, null rejection, copy-out to writable `.data`, and rejection of copy-out to read-only `.rodata`. NX mappings are enabled only after `IA32_EFER.NXE` is active.

`FILE_READ` validates the complete output range before consuming file bytes. `DIRECTORY_READ` does the same before advancing the directory cursor. `PIPE_READ` likewise validates the complete output range before removing bytes from the ring buffer; `PIPE_WRITE` copies user input into a bounded kernel buffer before modifying pipe state. Thus a rejected userspace pointer cannot silently consume file, directory, or pipe state.

## Spawn request

`SB_SYS_SPAWN_REQUEST` accepts the fixed 32-byte version-1 request:

```c
typedef struct {
    uint32_t size;
    uint16_t version;
    uint16_t source;
    uint64_t flags;
    uint64_t name;
    uint32_t name_length;
    uint32_t reserved;
} sb_spawn_request_t;
```

Implemented sources:

- `SB_SPAWN_SOURCE_BOOT_MODULE`: registered Multiboot image by name;
- `SB_SPAWN_SOURCE_VFS_PATH`: absolute VFS executable path, currently proven with `/boot/user-child`.

The kernel copies the request and source string into bounded kernel memory before lookup. Embedded NUL is rejected. Boot-module identifiers additionally reject whitespace; VFS sources require an absolute path.

Executable loading is source-independent after open: the VFS file is staged into kernel-owned memory, then passed to the generic ELF process loader. PID/TID values are allocated dynamically. Failed creation rolls back the partially created process/address-space state.

QEMU proves legacy selector spawn, versioned boot-module spawn, VFS-path spawn, WAIT/EXIT/reap, cross-process CR3 switching, and two dynamic child processes runnable concurrently.

## Handle model

Handles are process-local opaque 64-bit values. Userspace may store, compare, and pass them back, but must not decode slot/generation representation.

Each live entry contains object type, rights mask, kernel-only object pointer, optional close callback, and a generation used to reject stale values.

Current public object types include PROCESS, FILE, PIPE, EVENT, SHARED_MEMORY, SERVICE, and DIRECTORY. Defining a type does not imply every corresponding subsystem is complete.

Current rights are READ, WRITE, WAIT, SIGNAL, QUERY, and TRANSFER.

Closing a handle invalidates its generation before invoking the object-specific close callback. Reusing the slot therefore produces a distinct handle value. Old values return `SB_SYS_ERROR_STALE`.

Process teardown closes all remaining handles before destroying the user address space and collecting the process slot.

## FILE and DIRECTORY handles

A VFS node represents the resource; an open `sb_vfs_file_t` owns independent offset/access state and a node reference. `SB_SYS_FILE_OPEN` resolves an absolute path with READ (`1`), WRITE (`2`), or both (`3`), and grants the corresponding handle rights plus QUERY. Unknown/zero access is invalid. Boot modules and FAT read-only entries reject WRITE opens. QEMU proves `/boot/user-child` and `/disk/RUNTIME.TXT`.

`SB_SYS_FILE_WRITE` requires a live FILE handle with WRITE. Requests over 256 bytes return LIMIT. The complete source is copied from readable userspace before file data or offset changes; an invalid source returns FAULT. Zero length returns 0 after handle/rights validation without inspecting the pointer. A successful short write advances the offset by accepted bytes only; callers retry the remainder. FAT32 writes may extend EOF; seeking a hole, insufficient allocation space or size overflow returns LIMIT before mutation.

`SB_SYS_FILE_CREATE` takes an absolute path and access flags matching FILE_OPEN, with WRITE required. It returns a new FILE handle with QUERY plus requested rights, or EXISTS for an existing name without truncation. The whole path is copied before mutation; heap and handle slots are reserved before directory publication. Parent directories must exist, and writable FAT32 supports the ASCII 8.3 subset described in FILESYSTEM.md. Full directories grow by one cluster; insufficient allocation space or node-cache capacity returns LIMIT. In an existing slot the new entry is cached until FILE_SYNC; a grown directory flushes its initialized new cluster before mirrored FAT links and returns only after the FAT barrier. Writing the empty file allocates file-data clusters through the extension path. Creation does not implicitly truncate, overwrite, or create directories.

DIRECTORY handles use READ|QUERY. `SB_SYS_DIRECTORY_READ` copies a stable public 80-byte entry and reports EOF as `SB_SYS_ERROR_NOT_FOUND` without advancing the cursor. QEMU verifies a rejected read-only userspace output pointer does not consume the first FAT32 directory entry.

When FAT32 mounts with recorded unclean-shutdown, hard-error or inconsistent
mirrored status, writable FILE_OPEN, FILE_CREATE and DIRECTORY_CREATE return
RIGHTS. Existing reads, directory enumeration and read-handle FILE_SYNC remain
available. Sync does not clear the status or repair the filesystem. This gate
records dirty status before SuiraBox's first mutation in a mount session.
Failure to persist the marker returns IO and quarantines further writes; FILE_WRITE
reports zero accepted bytes. FILE_SYNC, CLOSE and successful DIRECTORY_CREATE do
not clear the marker. Explicit VOLUME_UNMOUNT can establish clean status after
all accepted changes are durable; otherwise the next mount requires recovery.
Atomic recovery and system shutdown remain future work. Existing numbers and
public layouts remain compatible; VOLUME_UNMOUNT is appended as number 41.

`SB_SYS_VOLUME_UNMOUNT` currently permits only bootstrap init (PID 1), the owner
of the global mount namespace; children receive RIGHTS before pointer access.
For init, reserved `rdx` must be zero, pointer/length must be nonzero, length must
be at most 255, and the whole absolute path is copied before finalization.
Bad mapping returns FAULT, excessive length LIMIT, invalid path/flags INVALID.
Path normalization is allowed, but only an exact mount point is removed: a file
path or missing mount returns NOT_FOUND. Providers without a finalizer, including
the /boot module provider, return INVALID and remain mounted.

FAT32 returns BUSY while any file/directory/borrowed node or mount alias remains;
nested mounts also return BUSY. No data is flushed or sealed on a BUSY refusal.
After handles close, it seals writes, flushes data/metadata, saves clean status
and flushes again before removing the mount. FILE_SYNC is not required first.
I/O failure returns IO and leaves the namespace readable, with mutation and
further finalization quarantined. A successful detach makes subsequent opens
under that mount NOT_FOUND unless another mounted parent supplies the path.
Read-only recovery mounts detach without disk I/O or status repair. The call
does not power off, remount or implicitly close handles. Kernel forceful reset
and object destruction are distinct from this explicit operation.

`SB_SYS_DIRECTORY_CREATE` creates one directory in an existing parent and returns
a DIRECTORY handle with READ|QUERY at cursor zero. Reserved flags (`rdx`) must be
zero. Paths use the same normalization and ASCII 8.3 restrictions as FILE_CREATE;
trailing slash and terminal dot components are rejected. A pre-existing file,
directory, or mount root returns EXISTS. Missing parents are not created.
The full path is copied and heap/handle capacity reserved before mutation.
Read-only parents/providers return RIGHTS. Child contents, allocation, and parent
entry are flushed before success. Physical `.`/`..` entries are hidden by
DIRECTORY_READ. Early I/O failures roll back allocation and allow retry; an
uncertain parent-publication barrier or failed rollback returns IO and quarantines
mount writes. In that case the name may exist despite the failed syscall and its
valid allocation is retained for repair. Creation is not power-loss atomic.

`SB_SYS_FILE_SYNC` requires QUERY and reaches `sb_vfs_file_sync()`, FAT32 mount sync, dirty block-cache writeback and the device flush barrier. WRITE success alone does not promise durability, and CLOSE does not implicitly sync. FAT32 supports overwrite and extension of existing files. Extension flushes data, then mirrored FATs, before accepting directory size; FILE_SYNC persists that final metadata. Extension errors leave offset/size unchanged but may modify overlapping existing bytes. Failed FAT rollback quarantines writes and file sync until repair/remount. Atomic replacement remains future work.

`SB_SYS_FILE_RENAME` publishes an existing regular file under a different ASCII
8.3 name within the same resolved parent directory. Both complete absolute
paths are copied before mutation. Reserved flags must be zero; pointer mapping,
length and embedded-NUL checks use the existing FAULT/LIMIT/INVALID contract.
Trailing slash and terminal dot components are invalid. Source or destination
mount roots are protected with RIGHTS. The call uses the same current global
filesystem authority as FILE_CREATE; it is not restricted to PID 1.

The destination must be absent (EXISTS otherwise). Different parents/volumes,
directory sources, LFN-associated source entries and unsupported providers return
INVALID; missing source/parent returns NOT_FOUND. Read-only sources, parents,
devices and recovery mounts return RIGHTS. A valid case-folded same-name request
is a no-op without writes or flushes. Physical entry location, allocation, size,
attributes and timestamps are retained, so held handles preserve their node,
offset and ability to write under the new name.

FAT32 persists dirty status before mutation, flushes staged contents and old-name
metadata, changes the short-name bytes, and completes another device flush before
success. A preflight read failure is retryable. Marker/publication I/O uncertainty
returns IO, retains dirty evidence and quarantines further mutation and clean
unmount; the new name may already be visible or durable. Successful rename does
not restore clean status. This call does not replace an existing destination,
move directories, synthesize LFN entries or guarantee power-loss atomicity.
Atomic replacement and a recovery protocol remain future work.

## Display surface

The append-only display calls use layouts in `include/suirabox/display_abi.h`.
DISPLAY_INFO is available to any process; it returns a zero-initialized 32-byte
`sb_display_info_t` containing version 1, width, height, RGB888 source format and
a maximum of 256 pixels per present. Reserved fields are zero. The interface
does not expose physical addresses, native pitch, channel masks or kernel MMIO.
No available mapped boot display returns NOT_FOUND. Invalid length/null pointer
or reserved `rdx` returns INVALID; an unwritable output mapping returns FAULT.

DISPLAY_PRESENT currently requires bootstrap init/PID 1. Children receive RIGHTS
before request access, including when the pointer is invalid. Its 32-byte request
contains size (u32), version (u16), flags (u16, zero), x/y/width/height (u32 each),
and a pixels pointer (u64). Rows are tightly packed, with one 32-bit word per
pixel in numeric 0x00RRGGBB form; the high byte is ignored. No alpha blending is
performed. Width and height must be positive, total pixels at most 256, and the
whole rectangle must fit the display. Null/invalid fields, flags, versions and
coordinates return INVALID; excess pixel count returns LIMIT. Absent mapped
display returns NOT_FOUND. Unreadable request or source mapping returns FAULT.

The kernel copies the complete request and bounded source before touching the
framebuffer, so user pointer faults cannot cause partial drawing. Native RGB565,
24-bit and 32-bit channel layouts are converted inside the validated surface
layer, preserving row padding. Present requests reject overflow rather than
clipping. Kernel fill helpers may clip. This is one boot framebuffer with one
authorized presenter, not buffer handles, window ownership, a compositor, text
rendering, GPU acceleration, input events or dynamic mode setting. A dedicated
display-service capability remains future work.

## PIPE handles

`SB_SYS_PIPE_CREATE` creates one kernel ring buffer and returns a fixed 16-byte pair of opaque process-local handles:

```c
typedef struct {
    sb_handle_t read_handle;
    sb_handle_t write_handle;
} sb_pipe_handles_t;
```

Both handles have type PIPE but expose role-specific rights:

- read endpoint: `READ|WAIT|QUERY`;
- write endpoint: `WRITE|WAIT|QUERY`.

The current ABI is deliberately nonblocking. Reading an empty pipe while a writer remains returns `SB_SYS_ERROR_WOULD_BLOCK`; writing a full pipe returns the same error. When the final writer closes, buffered data remains readable and a subsequent empty read returns 0 as EOF. Writing after the final reader closes returns `SB_SYS_ERROR_CLOSED`.

The phase-1 pipe uses a fixed 4096-byte kernel ring buffer and supports partial transfers. Endpoint close callbacks update reader/writer counts and release the shared pipe object after the last endpoint closes. QEMU verifies endpoint rights, empty-read backpressure, exact `PIPEPING` payload transfer, writer-close EOF, handle close, and stale-generation rejection.

Blocking pipe semantics are intentionally deferred until the wait-object layer has cross-task object sharing/wake semantics; the ring-buffer core itself does not embed scheduler policy.

## EVENT handles and timed waits

`SB_SYS_EVENT_CREATE` returns a manual-reset EVENT handle with `WAIT|SIGNAL|QUERY` rights. The initial state is explicitly unsignaled (`0`) or signaled (`1`).

`SB_SYS_EVENT_WAIT` behaves as follows:

- signaled event: returns 0 immediately;
- unsignaled + timeout `0`: returns `SB_SYS_ERROR_WOULD_BLOCK` without changing scheduler state;
- unsignaled + timeout `>0`: records one waiter, marks the current user task BLOCKED, saves a timer deadline, and immediately reschedules another runnable frame;
- deadline expiry: timer code overwrites the blocked task's saved syscall-frame `RAX` with `SB_SYS_ERROR_TIMEOUT`, makes it READY, and later `iretq` resumes immediately after the original `int 0x80`.

`SB_SYS_EVENT_SIGNAL` sets the manual-reset signal state and, if a still-blocked waiter is registered, completes that wait with result 0. `SB_SYS_EVENT_RESET` clears the signal state.

The current event core intentionally supports one registered waiter and is single-CPU/non-SMP-safe. QEMU currently proves the real timed BLOCKED/resume path, manual signal state, immediate signaled wait, reset, close, and stale-handle behavior. A separate task signaling an already-blocked waiter is not yet part of the userspace QEMU proof, because process-local handle transfer/multi-thread handle sharing is not yet exposed.

## Current filesystem/VFS proof

The current system namespace exposes `/boot` for registered Multiboot modules and `/disk` for the FAT32 runtime disk when present.

FAT32 supports 8.3 lookup, directory iteration, nested subdirectories, reads, exclusive file/directory creation with automatic parent-directory growth, regular-file overwrites and extension on writable devices. File extension allocates at most eight clusters per backend request; directory growth adds one cluster per create, and each new directory owns one initialized cluster. The syscall's 256-byte I/O limit remains unchanged. Seeking beyond EOF is rejected. LFN is not implemented.

The canonical block layer includes a fixed write-back sector cache. Full-sector writes become dirty cache entries, reads observe dirty data immediately, explicit flush/device unregister/replacement performs writeback, and a failed writeback preserves dirty state for retry. `sb_block_flush()` and `sb_vfs_sync()` provide the current synchronization boundary.

## Compatibility policy

- Existing syscall numbers are never renumbered within an ABI version.
- New syscalls are appended to the public maximum.
- Public handle type/right values remain stable once exposed.
- Handle slot/generation encoding is kernel-private.
- Versioned request structs use explicit `size` and `version` fields.
- Semantic changes that invalidate existing userspace require a new ABI version.
- In-tree kernel and userspace compile against the same public headers.
- CI treats any serial `Exception:` record as a hard regression.
