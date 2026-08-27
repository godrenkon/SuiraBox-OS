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

The first implementation is deliberately small and testable. It manages 4 KiB physical pages using a bitmap and exposes allocation/free counters.

This is a bootstrap allocator, not the final performance design.

## Planned evolution

1. Parse the Multiboot2 memory map.
2. Reserve kernel/image/modules/bootloader-owned regions.
3. Initialize the PMM from genuinely usable memory ranges.
4. Add x86_64 page-table management.
5. Create isolated process address spaces.
6. Add kernel heap allocation.
7. Add cache-aware and large-page strategies where benchmarks justify them.
8. Add resource accounting for Minecraft and server profiles.

## Safety rules

- Never treat all physical RAM as usable.
- Keep firmware-reserved and device/MMIO regions unavailable to normal allocation.
- Validate page alignment and allocator ownership on every public PMM operation.
- Do not expose physical pages directly to user applications without an explicit mapping policy.

## Performance direction

The target is predictable latency and efficient memory use rather than simply maximizing allocated bandwidth. JVM and Minecraft optimizations should be measured against representative workloads.
