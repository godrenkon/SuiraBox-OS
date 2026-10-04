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
| `-12` | `SB_SYS_ERROR_BUSY` | service already has an active client |

## Version 1 syscall table

The current public maximum syscall number is **38**.

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

Syscall 4 and syscall 13 are retained for ABI-v1 compatibility. New code should prefer the versioned spawn request and generic VFS path interfaces.

## ABI info routing

The original frame dispatcher owns the historic 0..16 implementation table. Object-specific calls 17 and above are append-only extensions routed by the syscall entry layer. This split is internal only: userspace sees one ABI and `SB_SYS_ABI_INFO` reports the public maximum, 38.

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

DIRECTORY handles use READ|QUERY. `SB_SYS_DIRECTORY_READ` copies a stable public 80-byte entry and reports EOF as `SB_SYS_ERROR_NOT_FOUND` without advancing the cursor. QEMU verifies a rejected read-only userspace output pointer does not consume the first FAT32 directory entry.

`SB_SYS_FILE_SYNC` requires QUERY and reaches `sb_vfs_file_sync()`, FAT32 mount sync, dirty block-cache writeback and the device flush barrier. WRITE success alone does not promise durability, and CLOSE does not implicitly sync. FAT32 supports overwrite and extension of existing files. Extension flushes data, then mirrored FATs, before accepting directory size; FILE_SYNC persists that final metadata. Extension errors leave offset/size unchanged but may modify overlapping existing bytes. Failed FAT rollback quarantines writes and file sync until repair/remount. Creation and atomic replacement remain future work.

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

FAT32 supports 8.3 lookup, directory iteration, nested subdirectories, reads, regular-file overwrites and extension on writable devices. Extension allocates at most eight clusters per backend request; the syscall's 256-byte limit remains unchanged. Seeking beyond EOF is rejected. LFN and file creation are not implemented.

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
