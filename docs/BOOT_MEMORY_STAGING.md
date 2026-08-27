# Staged Memory Bootstrap

SuiraBox uses a staged memory initialization strategy during early development.

## Stage 1: bootstrap PMM

The kernel initializes a small, known-safe physical RAM window before consuming the full Multiboot memory map. The bootstrap allocator is used for VMM, heap, and early process structures.

## Stage 2: memory-map integration

The Multiboot memory map must not be imported by calling a resetting PMM initializer after allocations exist. The final implementation must merge discovered usable/reserved ranges into the existing allocator while preserving all pages already allocated or reserved by the kernel.

The merge operation must:

- validate the Multiboot information address and tag sizes;
- reject malformed or overflowing records;
- preserve kernel/image/boot-module reservations;
- preserve pages already allocated by the bootstrap allocator;
- never reset the allocator underneath live allocations;
- run with bounded work;
- report incomplete hardware memory information as a non-fatal diagnostic when possible.

## Stage 3: normal memory manager

After the safe map merge, the allocator can expand to the detected physical RAM range and expose the full PMM API to later subsystems.

This separation is required so a malformed firmware memory map cannot prevent the OS from reaching the desktop bootstrap path.
