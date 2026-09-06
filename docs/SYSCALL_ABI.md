# SuiraBox Syscall ABI

## Version

Current userspace ABI version: **1**.

The canonical numeric definitions live in `include/suirabox/syscall_abi.h`. Handle metadata/types/rights live in `include/suirabox/handle_abi.h`. Kernel and userspace include the same public headers; syscall numbers and public handle constants must not be duplicated as private magic numbers.

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

The entry stub saves the complete GPR state in the same `sb_irq_frame_t` format used by interrupt-driven scheduling. This is required because a blocking syscall can return from the entry path using a different task's saved frame. When the original task later wakes, the exact saved syscall frame can be selected and restored before `iretq`.

`rflags`, `rip`, user `rsp`, and user `ss` are restored by `iretq`; userspace must not assume caller-saved registers other than the documented result in `rax` survive as an API guarantee. The kernel currently preserves the complete frame as an implementation property, but version 1 intentionally exposes only the registers above as the syscall contract.

## Return convention

Version 1 interprets `rax` as a signed 64-bit result:

- non-negative: success/result
- negative: ABI error

Current stable errors:

| Value | Name | Meaning |
| ---: | --- | --- |
| `-1` | `SB_SYS_ERROR_INVALID` | invalid syscall/argument/state |
| `-2` | `SB_SYS_ERROR_FAULT` | userspace pointer is unmapped or lacks required access |
| `-3` | `SB_SYS_ERROR_LIMIT` | request/table exceeds an ABI or kernel limit |
| `-4` | `SB_SYS_ERROR_STALE` | opaque handle generation is no longer valid |
| `-5` | `SB_SYS_ERROR_RIGHTS` | handle exists but lacks required rights |
| `-6` | `SB_SYS_ERROR_NOT_FOUND` | requested named resource/source does not exist |
| `-7` | `SB_SYS_ERROR_IO` | underlying object/provider I/O operation failed |

New error values may be appended. Existing meanings must not be silently changed within ABI version 1.

## Version 1 syscall table

| Number | Name | Arguments | Result |
| ---: | --- | --- | --- |
| 0 | `SB_SYS_GET_TICKS` | none | monotonic PIT tick count |
| 1 | `SB_SYS_PROCESS_ID` | none | current PID |
| 2 | `SB_SYS_EXIT` | `rdi=exit_code` | does not return on success |
| 3 | `SB_SYS_SLEEP` | `rdi=delay_ticks` | 0 after wake |
| 4 | `SB_SYS_SPAWN` | `rdi=image_selector` | child PID |
| 5 | `SB_SYS_WAIT_PROCESS` | `rdi=child_pid` | child exit code after wait |
| 6 | `SB_SYS_ABI_VERSION` | none | `SB_SYSCALL_ABI_VERSION` |
| 7 | `SB_SYS_LOG_WRITE` | `rdi=user_buffer`, `rsi=length` | bytes written |
| 8 | `SB_SYS_ABI_INFO` | `rdi=writable sb_syscall_abi_info_t*` | 0 and fills ABI info |
| 9 | `SB_SYS_PROCESS_OPEN_SELF` | none | opaque current-process handle with QUERY right |
| 10 | `SB_SYS_HANDLE_INFO` | `rdi=handle`, `rsi=writable sb_handle_info_t*` | 0 and fills public type/rights |
| 11 | `SB_SYS_HANDLE_CLOSE` | `rdi=handle` | 0; invalidates that handle generation |
| 12 | `SB_SYS_SPAWN_REQUEST` | `rdi=readable sb_spawn_request_t*` | dynamically allocated child PID |
| 13 | `SB_SYS_FILE_OPEN_BOOT_MODULE` | `rdi=readable name`, `rsi=name_length` | opaque read-only FILE handle |
| 14 | `SB_SYS_FILE_READ` | `rdi=file_handle`, `rsi=writable buffer`, `rdx=length` | bytes read |
| 15 | `SB_SYS_FILE_SEEK` | `rdi=file_handle`, `rsi=absolute_offset` | 0 on success |

`SB_SYS_SPAWN` remains a legacy ABI v1 bootstrap interface. Selector `SB_SPAWN_IMAGE_CHILD` resolves to the validation child image and remains operational so adding the general request path does not reinterpret or renumber an existing syscall.

`SB_SYS_FILE_OPEN_BOOT_MODULE` is intentionally transitional: it exposes the current Multiboot executable source through the generic VFS file-object and FILE-handle boundary. Filesystem/path-based open will replace the provider-specific entry point after VFS path/mount semantics exist; syscall 13 keeps its existing ABI meaning for compatibility.

## Userspace pointer rules

A syscall must never trust a ring3 pointer merely because its numeric value is in the canonical userspace range.

Before reading or writing userspace memory the kernel must:

1. reject range overflow and addresses outside `[SB_USER_BASE, SB_USER_LIMIT)`;
2. walk the target process page tables;
3. require `PRESENT|USER` at every level;
4. require effective `WRITABLE` at every level for kernel-to-user copies;
5. split copies at page boundaries and validate every touched page;
6. access the resolved physical mapping rather than directly dereferencing the untrusted userspace VA.

`SB_SYS_LOG_WRITE` proves the read side: QEMU CI requires a valid `.rodata` pointer to copy successfully and an invalid null pointer to return `SB_SYS_ERROR_FAULT` without causing a page fault.

`SB_SYS_ABI_INFO` proves the write side: QEMU CI requires a writable `.data` destination to receive `sb_syscall_abi_info_t`, then requires the same kernel-to-user copy aimed at `.rodata` to be rejected with `SB_SYS_ERROR_FAULT`. The userspace smoke program verifies the copied ABI version and maximum syscall number itself before continuing.

`SB_SYS_FILE_READ` additionally validates the complete destination range for write access **before** invoking the VFS object. This prevents a bad userspace output pointer from consuming file bytes or advancing the open-file offset. The read then lands in a bounded kernel buffer before copy-out. If that final copy unexpectedly fails after validation, the kernel restores the original file offset before returning `SB_SYS_ERROR_FAULT`.

## General spawn request

`SB_SYS_SPAWN_REQUEST` is the first non-selector process-launch interface. It accepts a fixed-size versioned request rather than assigning new meanings to legacy syscall 4.

Version 1 layout:

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

The structure is **32 bytes**. Current accepted values are:

- `size == SB_SPAWN_REQUEST_SIZE`;
- `version == SB_SPAWN_REQUEST_VERSION`;
- `source == SB_SPAWN_SOURCE_BOOT_MODULE`;
- `flags == SB_SPAWN_FLAG_NONE`;
- `reserved == 0`;
- `1 <= name_length <= SB_SPAWN_NAME_MAX`.

The kernel first copies the complete request into kernel-owned memory. It then separately copies exactly `name_length` bytes from the userspace `name` pointer into a bounded kernel buffer. Embedded NUL, space, and tab are rejected for the current boot-module identifier source, and the kernel appends its own terminator. The userspace pointer is never retained or passed directly to the Multiboot parser.

If the named registered boot module does not exist, the call returns `SB_SYS_ERROR_NOT_FOUND`. A successful request allocates process/thread identifiers in the kernel and returns the new PID. Userspace must treat returned PID values as dynamically assigned rather than assuming a fixed child number.

The process-creation path remains transactional: failure while creating the process, loading the ELF, creating its initial thread, or registering its scheduler task destroys the partially created process/address-space state.

### Current source limitation

Only `SB_SPAWN_SOURCE_BOOT_MODULE` is implemented today. The request contains an explicit source field so later VFS path or file-handle executable sources can be added without changing the meaning of existing fields. Therefore general process launch is substantially implemented, but full filesystem-backed spawn remains future work.

### QEMU lifecycle proof

The integration smoke keeps the legacy selector path first, then issues a separate `SB_SYS_SPAWN_REQUEST` for `user-child`. For the general request CI requires ordered evidence that:

1. the request and userspace name were copied/validated;
2. a child with dynamically allocated identifiers was created;
3. the parent actually entered a blocked WAIT for that child;
4. the child executed in ring3 and observed its dynamically assigned PID;
5. the child exited;
6. after wake, the kernel confirms the child process slot is no longer present (`process_get(pid) == NULL`), proving reap/collection completed;
7. the parent completed WAIT and continued execution.

The smoke then continues into the independent sleep/wake test, so successful spawn/wait cannot be satisfied by halting at the child lifecycle boundary.

## Handle model

Handles are process-local, opaque 64-bit values. Userspace may store, compare, and pass them back to the kernel, but **must not decode their bit representation**. The current kernel internally combines a table slot with a generation counter; that representation is deliberately excluded from the userspace ABI.

Every live handle entry carries:

- object type;
- rights mask;
- kernel-only object reference;
- kernel-only close callback/lifetime hook;
- generation state used to reject stale values.

Current public object types reserve values for process, file, pipe, event, shared memory, and service objects. Current rights reserve READ, WRITE, WAIT, SIGNAL, QUERY, and TRANSFER bits. Defining a type/right does not imply the corresponding subsystem is already implemented.

### Generation rule

Closing a handle invalidates its current generation before any object-specific close callback runs. If the table slot is later reused, the new handle value has a different generation. Passing the old value again returns `SB_SYS_ERROR_STALE`; it must never silently resolve to the new object occupying that slot.

### Rights rule

Kernel operations must request the rights they require when resolving a handle. `SB_SYS_HANDLE_INFO` requires QUERY. `SB_SYS_FILE_READ` and `SB_SYS_FILE_SEEK` require a FILE handle carrying READ. A valid handle with insufficient rights returns `SB_SYS_ERROR_RIGHTS` rather than bypassing the rights check.

### FILE object model

A VFS node describes the underlying resource: object type, size, capabilities, backend operations, private provider state, and reference count. An open VFS file is separate state containing a node reference, current offset, access mode, and open/closed state. Therefore two future opens of the same node can maintain independent offsets without putting per-open state into the filesystem node.

The current boot-module provider is read-only. It packages the provider node and its open-file state into one bootstrap allocation because the Phase 1 kernel heap still permits only one live heap allocation. That allocation strategy is an implementation limitation, not part of the userspace FILE ABI.

Closing the FILE handle invokes its kernel close callback. The open-file node reference is released, the provider object reaches reference count zero, and its backing bootstrap allocation is freed. Userspace never receives the VFS node pointer or provider pointer.

### Process teardown

Each process owns its handle table. After scheduler-owned execution resources are no longer active, process cleanup closes all remaining handles before destroying the user address space and collecting the process slot. This provides the resource-lifetime base needed by later file, pipe, event, shared-memory, and service handles.

### Current QEMU proof

The userspace smoke program proves both PROCESS and FILE handles. For FILE it:

1. opens the `user-child` boot module through the VFS provider and receives an opaque FILE handle;
2. queries it and verifies FILE type plus exact READ|QUERY rights;
3. attempts a 4-byte read into `.rodata` and requires `SB_SYS_ERROR_FAULT` before the VFS offset changes;
4. reads into writable `.data` and verifies the ELF magic at offset zero;
5. seeks back to zero and verifies the same ELF magic again;
6. closes the handle and verifies the old generation is stale.

A host handle test separately covers rights rejection, type rejection, table exhaustion, close callbacks, slot reuse with changed generation, stale lookup/close rejection, and close-all behavior. A VFS object host test covers independent open-file state, access enforcement, read/write/seek bounds, close behavior, and node lifetime.

## Compatibility policy

- Existing syscall numbers are never renumbered inside an ABI version.
- New syscalls are appended after `SB_SYS_MAX_NUMBER`.
- Public handle type/right numeric values are stable once exposed in an ABI version.
- Handle internal slot/generation encoding is **not** ABI and may change without userspace decoding it.
- Versioned request structs carry explicit `size` and `version` fields rather than silently changing layout semantics.
- A semantic change that invalidates existing userspace requires a new `SB_SYSCALL_ABI_VERSION`.
- Kernel and in-tree userspace are compiled against the same canonical headers.
- QEMU integration tests exercise version query, user-pointer boundaries, PROCESS/FILE handle generation semantics, legacy spawn compatibility, general spawn/wait/reap, and sleep/wake in sequence.
- CI treats any serial `Exception:` record as a hard regression even if later markers would otherwise appear.
