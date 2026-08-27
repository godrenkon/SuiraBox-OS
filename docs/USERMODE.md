# SuiraBox OS User Mode

## Current goal

Provide a controlled x86_64 ring 3 execution environment for userspace applications.

## Privilege layout

```text
Ring 0
  Kernel code/data
  Kernel heap
  Device drivers
  Scheduler
  System calls

Ring 3
  Applications
  SB Hub
  JVM
  Minecraft
  Browser
```

User processes receive their own page-table root. Kernel mappings remain supervisor-only, while user mappings are explicitly marked with the User/Supervisor permission bit.

## GDT selectors

- `0x18`: 64-bit kernel code
- `0x10`: kernel data
- `0x23`: 64-bit user code (DPL3)
- `0x2B`: user data (DPL3)
- `0x30`: TSS

## TSS

The TSS supplies `RSP0`, the kernel stack used when the CPU changes privilege level from ring 3 to ring 0 through an interrupt or exception.

The bootstrap kernel stack is used initially. Later each CPU will receive its own interrupt stack and TSS state.

## Transition policy

`arch_enter_user()` builds an `iretq` frame and is deliberately kept separate from process creation. A process must have a valid address space, code mapping, stack mapping, and syscall permissions before entering ring 3.

The final fast syscall path will use `SYSCALL/SYSRET` after user-mode execution is stable; the current `int 0x80` route is a bring-up interface.

## Security requirements

- User mappings must never gain supervisor-only access.
- Kernel pointers must not be trusted from userspace.
- Syscall arguments must be range-checked before dereference.
- Every process needs a distinct user address space.
- User code cannot modify page tables directly.
