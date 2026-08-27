# SuiraBox OS Userspace Architecture

## Goal

Userspace provides isolated execution for the desktop, runtime, Minecraft, Minecraft servers, and ordinary applications.

The kernel owns resources and policy enforcement. Applications must not access kernel memory, device registers, or other processes directly.

## Address-space model

```text
Process
  |
  +-- User text
  +-- Read-only data
  +-- Heap
  +-- Shared libraries / runtime
  +-- Thread stacks
  +-- Guard regions
  |
  +-----------------------------+
  | User virtual address space  |
  +-----------------------------+
  | Kernel mappings (protected) |
  +-----------------------------+
```

The initial architecture uses x86_64 four-level paging. User mappings use the page-table User/Supervisor permission bit; kernel mappings are supervisor-only.

## Process model

A process owns:

- an address space
- handles/file descriptors
- one or more threads
- resource limits/accounting
- a security identity

Threads own CPU execution state and scheduler metadata.

## Syscall ABI

The stable logical ABI is:

```text
syscall(number, arg0, arg1, arg2, arg3, arg4)
```

The bootstrap compatibility path may use `int 0x80`. A dedicated `SYSCALL/SYSRET` fast path is planned after user-mode entry and MSR setup are stable.

The kernel validates pointers, lengths, handles, and permissions at every syscall boundary.

## Initial syscall set

- `GET_TICKS`
- `PROCESS_ID`
- `EXIT`

The ABI is intentionally small. Filesystem, memory mapping, threads, synchronization, networking, and graphics syscalls will be added only after their kernel interfaces stabilize.

## User-mode entry

Before ring-3 execution is enabled, the kernel must have:

1. kernel/user GDT descriptors
2. a valid Task State Segment with a kernel stack for privilege transitions
3. user page tables
4. a user stack and guard page
5. IDT gates with correct privilege rules
6. a safe return path
7. syscall pointer/argument validation

## Security boundary

Ring-3 code is untrusted. User memory is never assumed valid merely because it is mapped.

Device access is mediated through kernel drivers and explicit handles. The filesystem, network, GPU, and other privileged subsystems must not expose raw hardware access to applications by default.

## Runtime implications

The JVM will run as a normal isolated userspace workload rather than inside the kernel.

Minecraft and Minecraft servers will therefore benefit from kernel scheduling, memory, storage, network, and graphics policy without making the kernel depend on Minecraft internals.

## Development order

```text
PMM / VMM
  -> process address spaces
  -> user/kernel permissions
  -> thread context
  -> ring-3 entry
  -> syscall ABI
  -> userspace loader
  -> native runtime
  -> JVM
```
