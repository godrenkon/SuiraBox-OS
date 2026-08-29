# SB Desktop Resource Rollout Roadmap

## Phase 1 — Core boundary

Keep the bootable Core self-contained:

- kernel and boot path
- memory/process/address-space infrastructure
- display/input/compositor/window primitives
- local storage/VFS/filesystem
- minimal settings and recovery terminal
- minimal fallback UI assets
- resource client, verifier, and cache primitives

No Optional Resource may be required for boot.

## Phase 2 — External resource catalog

Create the separate repository:

`godrenkon/SuiraBox-OS-Resources`

Initial resource families:

- locales
- themes
- icon packs
- wallpapers
- additional fonts
- optional applications
- sound/help/sample data

Keep manifest and payloads separate. The OS should need only a small trusted manifest channel to discover compatible resources.

## Phase 3 — Lazy installation

Implement:

`select -> manifest -> cache lookup -> download -> verify -> atomic activate`

Do not download resources merely because they exist in the catalog.

## Phase 4 — Core UI integration

The GUI exposes download/install states without depending on the downloaded resource being available.

Required states:

- available
- downloading
- verifying
- installed
- active
- unavailable
- corrupt
- incompatible

## Phase 5 — Cache management

Use content-addressed storage and deduplication. Keep only resources used by the current system/user plus explicitly pinned resources.

Generated caches are separately evictable.

## Phase 6 — Optional application packages

Only after process execution, storage, networking, package verification, and rollback are stable should large GUI applications move out of Core.

Minimal Settings and recovery Terminal remain Core.

## Acceptance criteria

The resource architecture is complete only when:

- Core boots with no network;
- Core can recover from a failed or corrupt download;
- Optional resources are never embedded in the base ISO;
- only selected resources are downloaded;
- duplicate payloads are deduplicated;
- resource activation is atomic;
- executable resources require stronger authorization/signature verification than a hash alone;
- removing Optional Resources does not damage Core functionality.
