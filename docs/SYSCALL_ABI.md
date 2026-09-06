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

`SB_SYS_SPAWN` is still a bootstrap interface: selector `SB_SPAWN_IMAGE_CHILD` resolves to a trusted boot module. It will become a validated path/descriptor-based launch interface after the VFS/handle boundary is ready.

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

Kernel operations must request the rights they require when resolving a handle. `SB_SYS_HANDLE_INFO` currently requires QUERY. A valid handle with insufficient rights returns `SB_SYS_ERROR_RIGHTS` rather than bypassing the rights check.

### Process teardown

Each process owns its handle table. After scheduler-owned execution resources are no longer active, process cleanup closes all remaining handles before destroying the user address space and collecting the process slot. This provides the resource-lifetime base needed by later file, pipe, event, shared-memory, and service handles.

### Current QEMU proof

The userspace smoke program:

1. opens a QUERY-only handle to its current process;
2. queries it and verifies `PROCESS` type + exact QUERY rights in userspace memory;
3. closes the handle;
4. queries the same opaque value again and requires `SB_SYS_ERROR_STALE`.

A host test separately covers rights rejection, type rejection, table exhaustion, close callbacks, slot reuse with changed generation, stale lookup/close rejection, and close-all behavior.

## Compatibility policy

- Existing syscall numbers are never renumbered inside an ABI version.
- New syscalls are appended after `SB_SYS_MAX_NUMBER`.
- Public handle type/right numeric values are stable once exposed in an ABI version.
- Handle internal slot/generation encoding is **not** ABI and may change without userspace decoding it.
- A semantic change that invalidates existing userspace requires a new `SB_SYSCALL_ABI_VERSION`.
- Kernel and in-tree userspace are compiled against the same canonical headers.
- QEMU integration tests exercise version query, read/write pointer boundaries, handle generation semantics, and process lifecycle in sequence.
- CI treats any serial `Exception:` record as a hard regression even if later markers would otherwise appear.
