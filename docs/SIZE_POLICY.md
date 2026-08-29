# SB Desktop Size and Performance Policy

Optimize the whole system rather than minimizing only the ISO byte count.

## Priority

1. Keep boot and recovery reliable.
2. Reduce always-resident RAM.
3. Reduce hot-path CPU work.
4. Reduce boot-time I/O.
5. Reduce ISO/storage footprint.
6. Externalize only user-selectable large payloads.

## Never trade away

- offline boot
- recovery
- basic GUI
- local storage access
- security/integrity verification
- the minimal network path needed to obtain Optional Resources

## Prefer

- lazy loading
- immutable shared resources
- content-addressed deduplication
- section garbage collection
- cached immutable geometry/format state
- damage-aware rendering
- bounded event queues and state machines
- direct/mapped access instead of whole-resource copies where safe
- generated cache eviction

## Avoid

- embedding every language/theme/wallpaper/font/application
- preloading optional resources at boot
- duplicate copies of the same payload
- large static lookup tables when small generated structures suffice
- busy-spin loops when the CPU can sleep
- keeping decode caches forever

The smallest useful Core is the goal, not the smallest possible binary at the cost of an unusable OS.
