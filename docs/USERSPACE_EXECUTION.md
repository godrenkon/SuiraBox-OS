# Userspace Execution Plan

SuiraBox userspace is being built as a small freestanding environment first. The kernel owns isolation, virtual memory, process/thread state, and syscalls; userspace owns applications and runtimes.

## Execution path

```text
ELF file
  ↓
ELF64 validation
  ↓
Create process
  ↓
Create address space
  ↓
Map PT_LOAD pages
  ↓
Map user stack
  ↓
Create initial thread
  ↓
Activate CR3
  ↓
iretq → ring 3
  ↓
userspace _start
  ↓
int 0x80 / future SYSCALL
```

## Initial user address layout

- text/data start near `0x0000004000000000`
- initial stack is placed near `0x0000004000100000`
- user mappings must carry the CPU user-accessible bit
- kernel mappings remain supervisor-only

## Security requirements

- validate every ELF header and program-header range before reading it
- reject non-canonical virtual addresses
- reject mappings outside the configured user range
- enforce `p_filesz <= p_memsz`
- derive page permissions from ELF segment flags rather than making all pages writable/executable
- never map kernel-owned pages as user-accessible
- keep user stack non-executable

## Performance direction

The early path should prioritize correctness. Later work can optimize process creation using copy-on-write, shared runtime pages, lazy mapping, and page-cache reuse. Those optimizations must be benchmarked using real JVM and Minecraft workloads.
