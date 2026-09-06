# SuiraBox Syscall ABI

## Version

Current userspace ABI version: **1**.

The canonical numeric definitions live in `include/suirabox/syscall_abi.h`. Kernel and userspace include the same header; syscall numbers must not be duplicated as private magic numbers.

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
| `-2` | `SB_SYS_ERROR_FAULT` | userspace pointer is unmapped or inaccessible |
| `-3` | `SB_SYS_ERROR_LIMIT` | request exceeds an ABI limit |

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

`SB_SYS_LOG_WRITE` is the first runtime proof of this boundary. QEMU CI requires a valid `.rodata` pointer to copy successfully and an invalid null pointer to return `SB_SYS_ERROR_FAULT` without causing a page fault.

## Compatibility policy

- Existing syscall numbers are never renumbered inside an ABI version.
- New syscalls are appended after `SB_SYS_MAX_NUMBER`.
- A semantic change that invalidates existing userspace requires a new `SB_SYSCALL_ABI_VERSION`.
- Kernel and in-tree userspace are compiled against the same canonical header.
- QEMU integration tests must exercise the version query before lifecycle tests, so an ABI mismatch cannot be hidden by later scheduler success.
