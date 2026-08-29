# SB Desktop Resource Policy

## Core

Core is shipped in the ISO and must work offline.

Keep these in Core:

- boot path and kernel
- PMM/VMM/heap/process/address-space/syscall/interrupt primitives
- display/framebuffer/input primitives
- minimal compositor/window manager
- storage/VFS/filesystem primitives
- networking primitives required to reach the Resource service
- Resource manifest verification/cache machinery
- minimal Settings and recovery UI
- minimal Terminal/recovery CLI
- minimal glyph/font fallback
- boot/recovery/error strings
- tiny default theme/chrome required to make the first GUI usable

Rule: if removing it can prevent boot, basic GUI use, recovery, storage access, or setup without network access, it stays Core.

## Optional Resource

Optional Resource is downloaded only when the user selects or needs it.

Good candidates:

- full locale packs
- large fonts
- additional themes
- icon packs
- wallpapers
- sound packs
- tutorials/help databases
- large sample data
- optional applications
- high-feature versions of applications already represented by a minimal Core recovery tool

Rule: a component may be externalized only when SB Desktop remains usable without it and the user can reasonably choose whether to install it.

## Generated Cache

Generated caches are never shipped in the ISO.

Examples:

- compiled glyph atlases
- decoded image caches
- shader/cache data if later required
- derived indexes
- temporary downloads

Caches must be deletable and reproducible.

## Applications

Do not move every application out of Core just to reduce the ISO.

Keep a minimal Settings UI and recovery Terminal/CLI in Core. A minimal storage/recovery UI should remain available even when the Resource repository is unreachable.

High-feature GUI applications may be Optional Resources when their absence does not block boot or recovery.

## Languages

Core contains only the tiny strings necessary to boot, recover, configure networking/storage, select a language, show errors, and continue setup.

Full translations are Optional Resources. The selected locale is downloaded on demand and cached by SHA-256 content identity.

A missing/corrupt locale must fall back to Core strings and must never block boot.

## Resource repository

Planned repository:

`godrenkon/SuiraBox-OS-Resources`

The repository is separate from the OS source repository. Optional payloads must not be embedded into the OS ISO.

The repository itself is not automatically created yet because the currently available GitHub integration does not expose repository-creation mutation.

## Download model

`selection -> trusted manifest -> cache lookup -> download only selected payload/dependencies -> hash verification -> atomic activation`

Never download all available resources.
Never preload optional resources during boot.
Never activate a payload before verification.

## Size/performance rule

Optimize the whole system, not only ISO size:

- minimize shipped code and data
- avoid duplicate payloads
- deduplicate cache objects by content hash
- lazy-load optional data
- memory-map large resources where practical
- keep generated caches outside the ISO
- avoid unnecessary boot-time I/O
- keep Core small enough that externalization does not become more expensive than the saved space

The goal is a small, fast, offline-capable Core rather than an empty ISO that becomes unusable without downloads.
