# SuiraBox Userspace

The userspace is intentionally freestanding during early development. It will eventually provide the native runtime, shell, services, and the JVM environment.

## ABI direction

- x86_64
- user code/data selectors are DPL3
- user virtual memory begins at `0x0000004000000000`
- initial syscall ABI is defined in `kernel/syscall.h`
- early syscall compatibility uses `int 0x80`
- the long-term fast path may use x86_64 `SYSCALL/SYSRET`

## First milestones

1. Build a tiny userspace ELF.
2. Load it with the kernel ELF loader.
3. Create its user address space and stack.
4. Enter ring 3.
5. Return through the syscall interface.
6. Add a minimal shell.
