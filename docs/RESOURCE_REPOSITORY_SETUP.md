# External Resource Repository Setup

## Intended repository

`godrenkon/SuiraBox-OS-Resources`

This repository is deliberately separate from `godrenkon/SuiraBox-OS`.

The OS repository must never require this repository to build the kernel or the baseline Desktop.

## What belongs there

- large locale packs beyond the Core language UI;
- large theme and icon collections;
- wallpaper collections;
- sound and notification packs;
- optional application bundles;
- large documentation datasets;
- feature-specific data such as Minecraft integration assets.

## What must stay in SuiraBox-OS

- kernel and boot files;
- hardware/runtime interfaces;
- filesystem and configuration recovery;
- Display/Input/Compositor/Window Manager/Desktop Shell;
- Settings / Terminal / basic File Manager;
- tiny fallback font/glyph set;
- tiny fallback theme/background;
- resource manifest parser, cache manager and integrity verifier.

## Repository rules

The Resource Repository should contain only data/packages and metadata required to describe those packages. It must not become a second operating system source tree.

Every package needs:

- stable resource ID;
- semantic version;
- resource type;
- minimum Resource ABI version;
- compressed and expanded size;
- SHA-256 digest;
- dependency IDs;
- package-relative path.

Large package collections should be independently releasable so a user can fetch one resource without downloading unrelated assets.

## Release channels

The repository should support at least:

- `stable`: released resources compatible with the stable SB Desktop ABI;
- `testing`: opt-in resources for development/testing.

SB Desktop defaults to `stable` and must not silently switch channels.

## Endpoint configuration

The endpoint is configuration, not a kernel compile-time constant. A future Settings/Resource screen may select a repository mirror while preserving the same manifest/package validation rules.

## Current state

The repository is intentionally not created automatically here because the available GitHub integration exposes repository lookup and file operations but not the repository-creation mutation. The OS implementation therefore treats the external repository as a future service dependency rather than pretending it already exists.
