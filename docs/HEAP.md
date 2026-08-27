# SuiraBox Kernel Heap

The bootstrap kernel heap sits above the PMM and existing identity-mapped VMM.

## Bootstrap design

The initial allocator intentionally supports only allocations that fit within one 4 KiB page. Each allocation is tracked in a small static record table so invalid frees are ignored and PMM ownership can be restored.

```text
Kernel code
    |
    v
Kernel heap
    |
    v
PMM 4 KiB pages
    |
    v
Physical memory
```

This is not the final allocator. It is a deterministic bootstrap layer that lets later kernel subsystems use dynamic memory without introducing a complex allocator too early.

## Planned evolution

- multi-page contiguous allocation in PMM
- first-fit/best-fit or segregated free-list allocator
- slab/slub-style caches for frequently allocated kernel objects
- per-CPU caches where useful
- guard pages and debug poisoning in development builds
- allocation tracing and leak detection
- NUMA-aware policies on supported hardware

Performance changes must be benchmarked. Minecraft/JVM workloads should not force kernel heap policy into application-specific assumptions.
