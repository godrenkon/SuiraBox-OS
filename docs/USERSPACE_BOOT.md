# Userspace boot path

The first userspace milestone is deliberately split into preparation and execution:

1. GRUB loads `user-hello.elf` as a Multiboot2 module.
2. The kernel finds the named module.
3. `process_prepare_boot_module()` validates and loads its ELF64 `PT_LOAD` segments.
4. A per-process address space is created with the kernel identity mapping kept supervisor-only.
5. A four-page NX user stack is mapped above the image.
6. The process records its entry point and user stack top.
7. Only after these checks pass will the architecture layer enter ring 3.

The userspace image is linked at `0x0000008000000000`, which starts PML4 slot 1. The bootstrap/kernel identity map remains in PML4 slot 0, so userspace mappings do not require setting the user-accessible bit on the kernel mapping tree.

## Security boundary

User mappings must remain below the canonical upper-half boundary and may only use the designated userspace PML4 slot. Kernel mappings are not copied into a user-accessible PML4 entry.

## Next execution milestone

Implement a complete user-thread trap frame and enter/return path. The scheduler must be able to distinguish kernel contexts from user contexts before preemptive user scheduling is enabled.
