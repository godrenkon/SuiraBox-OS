# SB Storage Hygiene

SuiraBox should minimize hidden disk use without risking user data.

## Principle

The base system owns only files required for boot, recovery, security, updates, hardware support, package management, and the selected desktop foundation. Optional applications and their private caches belong to their packages.

## Data classes

### Required system data

Must not be removed by normal cleanup:

- boot files
- kernel and init components
- package database
- recovery metadata
- security metadata and trusted keys
- active system configuration
- hardware firmware that is required by an installed driver

### Rebuildable system data

Can be regenerated safely when not in active use:

- package download cache
- generated indexes
- compiler caches
- thumbnail caches
- shader caches
- temporary extraction directories
- stale crash dumps after explicit retention rules

### User data

Never remove silently:

- documents
- saves
- Minecraft worlds
- screenshots
- recordings
- project files
- server worlds and configuration
- application data not explicitly owned by a removable package

### Optional package data

A package must declare which files, services, startup entries, caches, and shared dependencies it owns. Uninstall can remove owned data only after dependency and user-data checks.

## Garbage collection

The system should expose a storage analyzer with:

- reclaimable cache size
- orphaned package files
- duplicate package versions
- unused runtimes
- unused language/toolchains
- old recovery snapshots
- large user data categories

Cleanup has three modes:

- Safe: only known rebuildable data
- Recommended: safe data plus confirmed orphaned package data
- Expert: user-selected paths with explicit confirmation

Safe cleanup must not depend on filename guessing alone. Ownership metadata and package records are authoritative.

## Runtime behavior

Optional services should be on-demand. Installing software must not automatically create permanent background processes unless the package declares a required service and the user accepts it.

Caches use size limits and age policies. The OS should prefer deleting cache before user data when storage pressure occurs.

## Low-spec mode

SB may expose a Low Spec profile that reduces:

- default service count
- cache retention
- animation and visual effects
- background indexing
- prefetch activity
- resident helper processes

This changes policy and resource usage; it does not claim that unsupported hardware can run software below its actual requirements.
