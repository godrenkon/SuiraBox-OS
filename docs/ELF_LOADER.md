# SuiraBox ELF64 Loader

The userspace loader accepts ELF64 x86_64 executables and maps only validated loadable segments into a process address space.

## Load pipeline

```text
File / VFS
   ↓
ELF validator
   ↓
Program headers
   ↓
Segment policy checks
   ↓
Physical pages from PMM
   ↓
User mappings in process address space
   ↓
Initial user stack
   ↓
User entry point
```

## Validation rules

- ELF magic, class, endianness, machine and version must match.
- Program-header offsets and sizes must remain inside the image.
- `p_filesz` must not exceed `p_memsz`.
- Virtual-address ranges must be page aligned when required by the loader policy.
- The entry point must fall inside a mapped executable load segment.
- User mappings must remain inside the reserved userspace range.
- Integer overflows must be rejected rather than wrapped.

## Segment permissions

ELF program-header flags are translated to page permissions:

```text
PF_R  -> readable
PF_W  -> writable
PF_X  -> executable
```

Executable pages should not be writable unless explicitly permitted by a future compatibility policy.

## Loading policy

The first implementation will load ordinary `ET_EXEC` and position-independent `ET_DYN` files with ASLR deferred until the basic loader is stable.

The loader will not invoke arbitrary code while validating an image.

## Runtime relation

The loader is independent of the JVM. Native applications and later JVM launchers both use the process/address-space primitives supplied by the kernel.
