# SuiraBox OS Memory Architecture

## Goals

Memory management must support a general-purpose desktop OS and later provide the primitives needed by a JVM and Minecraft workloads.

The design is layered:

```text
Hardware memory map
      |
      v
Physical Memory Manager (PMM)
      |
      v
Virtual Memory Manager (VMM)
      |
      +-- Kernel mappings
      +-- User address spaces
      |
      v
Kernel heap / allocators
      |
      v
Processes / Runtime / JVM
```

## Initial PMM

The first implementation manages 4 KiB physical pages using a bitmap and exposes allocation/free counters.

For the bootstrap, the allocator tracks the first 64 MiB of physical memory. Pages start reserved; ranges reported as available by the Multiboot2 memory map are released, and the linked kernel image range is reserved again afterward.

This is a bootstrap allocator, not the final performance design.

## Initial VMM

The x86_64 VMM uses the standard four-level page-table hierarchy:

```text
PML4 -> PDPT -> PD -> PT -> 4 KiB page
```

The early boot code establishes a 0–1 GiB identity map with 2 MiB pages. The VMM reuses that address space and allocates lower-level tables on demand for new 4 KiB mappings.

Initial operations:

- map a 4 KiB virtual page
- translate a virtual address
- unmap a 4 KiB page
- invalidate a changed mapping with `invlpg`

The bootstrap VMM does not split an existing 2 MiB large-page leaf. New test mappings therefore use an address outside the current large-page identity map.

## Planned evolution

1. Improve Multiboot2 memory-map validation and reservation coverage.
2. Add isolated process address spaces.
3. Add guard pages and stronger page-permission enforcement.
4. Add kernel heap allocation.
5. Add copy-on-write and shared-memory facilities.
6. Evaluate buddy allocation, per-CPU caches, huge pages, and NUMA-aware policies.
7. Add DMA/IOMMU-aware memory isolation.
8. Add resource accounting and workload-aware policies for Minecraft and JVM workloads.

## Safety rules

- Never treat all physical RAM as usable.
- Keep firmware-reserved and device/MMIO regions unavailable to normal allocation.
- Validate page alignment and allocator ownership on every public PMM operation.
- Do not expose physical pages directly to user applications without an explicit mapping policy.

## Performance direction

The target is predictable latency and efficient memory use rather than simply maximizing bandwidth. JVM and Minecraft optimizations should be measured against representative workloads and compared with reproducible baselines.
