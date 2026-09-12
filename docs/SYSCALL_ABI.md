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

## Version 1 syscall table

The current public maximum syscall number is **21**.

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

Syscall 4 and syscall 13 are retained for ABI-v1 compatibility. New code should prefer the versioned spawn request and generic VFS path interfaces.

## ABI info routing

The original frame dispatcher owns the historic 0..16 implementation table. Object-specific calls 17 and above are append-only extensions routed by the syscall entry layer. This split is internal only: userspace sees one ABI and `SB_SYS_ABI_INFO` reports the public maximum, 21.

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

Each live entry contains:

- object type;
- rights mask;
- kernel-only object pointer;
- optional close callback;
- generation used to reject stale values.

Current public object types include PROCESS, FILE, PIPE, EVENT, SHARED_MEMORY, SERVICE, and DIRECTORY. Defining a type does not imply every corresponding subsystem is complete.

Current rights are READ, WRITE, WAIT, SIGNAL, QUERY, and TRANSFER.

Closing a handle invalidates its generation before invoking the object-specific close callback. Reusing the slot therefore produces a distinct handle value. Old values return `SB_SYS_ERROR_STALE`.

Process teardown closes all remaining handles before destroying the user address space and collecting the process slot.

## FILE handles

A VFS node represents the resource; an open `sb_vfs_file_t` owns independent offset/access state and a node reference. FILE handles currently use READ|QUERY for the read-only paths proven in CI.

`SB_SYS_FILE_OPEN` resolves an absolute path through the system namespace. QEMU proves both `/boot/user-child` and a real FAT32 disk path `/disk/RUNTIME.TXT`.

The runtime disk test uses the full path:

`QEMU IDE -> ATA PIO -> block device -> FAT32 -> system VFS /disk -> FILE handle -> userspace read`.

FILE close releases the VFS object and heap-owned open state through the handle close callback.

The VFS object layer also exposes `sb_vfs_file_sync()`. Writable backends may provide a node-level sync callback; the block/VFS layer already provides explicit device/mount flush operations. The current FAT32 runtime mount remains read-only, so userspace fsync and atomic update semantics are not yet complete.

## DIRECTORY handles

`SB_SYS_DIRECTORY_OPEN` resolves an absolute directory path through the same system VFS namespace and returns a DIRECTORY handle with READ|QUERY rights.

`SB_SYS_DIRECTORY_READ` copies a stable public 80-byte entry:

```c
typedef struct {
    uint32_t type;
    uint16_t name_length;
    uint16_t reserved;
    uint64_t size;
    char name[64];
} sb_directory_entry_t;
```

Entry type values are regular file, directory, or device. `name` is one component, not a path. EOF is reported as `SB_SYS_ERROR_NOT_FOUND` and does not advance the cursor.

The kernel validates the destination for the full 80 bytes before calling the VFS directory iterator. QEMU deliberately supplies a read-only output pointer first, requires `SB_SYS_ERROR_FAULT`, then verifies the first real FAT32 entry is still `RUNTIME.TXT`. It subsequently verifies EOF, close, and stale-generation behavior.

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

Blocking pipe semantics are intentionally deferred until the generic event/wait-object layer can wake scheduler-blocked syscall frames without embedding scheduler policy inside the ring-buffer core.

## Current filesystem/VFS proof

The current system namespace exposes:

- `/boot`: registered Multiboot modules through a VFS directory provider;
- `/disk`: read-only FAT32 mounted from the QEMU primary IDE disk when available.

FAT32 currently supports 8.3 lookup, directory iteration, nested subdirectories, and read-only regular-file access. LFN/write support is not implied by the current ABI.

The canonical block layer now includes a fixed write-back sector cache. Full-sector writes become dirty cache entries, reads observe dirty data immediately, explicit flush/device unregister/replacement performs writeback, and a failed writeback preserves dirty state for retry. `sb_block_flush()` and `sb_vfs_sync()` provide the current synchronization boundary.

## Compatibility policy

- Existing syscall numbers are never renumbered within an ABI version.
- New syscalls are appended to the public maximum.
- Public handle type/right values remain stable once exposed.
- Handle slot/generation encoding is kernel-private.
- Versioned request structs use explicit `size` and `version` fields.
- Semantic changes that invalidate existing userspace require a new ABI version.
- In-tree kernel and userspace compile against the same public headers.
- CI treats any serial `Exception:` record as a hard regression.