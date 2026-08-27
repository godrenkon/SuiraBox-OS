# SuiraBox syscall ABI

SuiraBox exposes a small stable syscall ABI to userspace. The initial compatibility
entry is `int 0x80`; a future fast path may use x86_64 `SYSCALL/SYSRET` without
changing the logical syscall numbers.

## Initial calls

| Number | Name | Result |
|---:|---|---|
| 0 | `SB_SYS_GET_TICKS` | monotonic scheduler timer ticks |
| 1 | `SB_SYS_PROCESS_ID` | current process/thread owner id |
| 2 | `SB_SYS_EXIT` | terminate the current process (placeholder in bootstrap) |

## ABI

- `rax`: syscall number
- `rdi`, `rsi`, `rdx`, `r10`, `r8`: arguments 0..4
- `rax`: return value

Userspace must not access kernel addresses directly. File, network, graphics and
Minecraft management services will be exposed through higher-level userspace APIs.
