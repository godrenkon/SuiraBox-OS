# Staged Memory Bootstrap

SuiraBox uses a staged memory initialization strategy during early development.

## Stage 1: bootstrap PMM

The kernel initializes a small, known-safe physical RAM window before consuming the full Multiboot memory map. The bootstrap allocator is used for VMM, heap, and early process structures.

## Stage 2: memory-map integration

The Multiboot memory map is imported by merging it into the bootstrap allocator; it must never reset the PMM. PMM tracks dynamically allocated pages separately from permanently reserved pages, so a map merge cannot release either kind of live ownership.

The merge operation must:

- validate the Multiboot information address and tag sizes;
- reject malformed or overflowing records;
- reserve the kernel image, live Multiboot information and boot modules after usable ranges are imported;
- preserve pages already allocated by the bootstrap allocator;
- never reset the allocator underneath live allocations;
- run with bounded work;
- report incomplete hardware memory information as a non-fatal diagnostic when possible.

## Stage 3: normal memory manager

After the safe map merge, the allocator can expand to the detected physical RAM range and expose the full PMM API to later subsystems.

This separation is required so a malformed firmware memory map cannot prevent the OS from reaching the desktop bootstrap path.
