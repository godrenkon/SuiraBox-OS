# SuiraBox Userspace

The userspace is intentionally freestanding during early development. It will eventually provide the native runtime, shell, services, and the JVM environment.

## ABI direction

- x86_64
- user code/data selectors are DPL3
- user virtual memory begins at `0x0000004000000000`
- initial syscall ABI is defined in `kernel/syscall.h`
- early syscall compatibility uses `int 0x80`
- the long-term fast path may use x86_64 `SYSCALL/SYSRET`

## Current shell ABI

`userspace/shell.h` exposes the stable minimal shell dispatcher:

```c
uint64_t sb_shell_command_count(void);
const char *sb_shell_command_name(uint64_t index);
uint64_t sb_shell_run_command(uint64_t index);
```

The current command table contains `ticks` and `pid`. Invalid command indexes return `UINT64_MAX` from `sb_shell_run_command()` and a null pointer from `sb_shell_command_name()`.

The dispatcher does not perform console I/O yet. Terminal input/output remains a separate subsystem so the shell ABI does not become coupled to one device implementation.

## First milestones

1. Build a tiny userspace ELF.
2. Load it with the kernel ELF loader.
3. Create its user address space and stack.
4. Enter ring 3.
5. Return through the syscall interface.
6. Add a minimal shell.
7. Add the terminal input/output layer.
8. Connect the shell to the terminal without bypassing process or memory-safety boundaries.
