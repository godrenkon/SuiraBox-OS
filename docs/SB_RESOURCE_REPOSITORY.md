# SB Resource Repository Architecture

## Purpose

SB Desktop is intentionally split into a minimal base system and separately distributed resources. The base image must contain only resources required to boot, diagnose, configure, and reach a usable desktop. Optional resources are fetched on demand.

## External repository

Planned repository: `godrenkon/SuiraBox-OS-Resources`

The OS resource registry currently points at:

`https://raw.githubusercontent.com/godrenkon/SuiraBox-OS-Resources/main/manifest/manifest-v1.json`

The repository itself is a separate distribution artifact and must not be vendored into the OS repository.

## Resource classes

The external repository may contain:

- `locale`: language catalogs and locale metadata;
- `keyboard`: keyboard layout data;
- `font`: optional multilingual fonts and fallback families;
- `theme`: theme definitions;
- `wallpaper`: backgrounds and related metadata;
- `icon`: icon packs;
- `sound`: UI/event sounds;
- `app`: optional applications and system applications that are not required to boot;
- `document`: help/manual content.

The OS must keep only the smallest fallback assets needed before network/resource installation is available.

## Distribution rule

A resource must be separated when doing so materially improves at least one of:

- ISO size;
- installed disk usage;
- RAM usage;
- boot/startup time;
- update granularity;
- security isolation;
- maintainability;
- user choice.

Do not split components into meaningless micro-packages.

## Manifest v1

Each resource reference contains:

- stable `id`;
- resource `type`;
- relative payload `path`;
- semantic `version`;
- minimum compatible SB API;
- compressed size;
- expanded size;
- SHA-256 digest;
- dependency IDs;
- distribution tier.

The client must validate all fields before activation.

## Download lifecycle

```text
UNAVAILABLE
  -> AVAILABLE
  -> DOWNLOADING
  -> VERIFYING
  -> INSTALLED
  -> ACTIVE
```

Failures transition to `CORRUPT`, `INCOMPATIBLE`, or `UNAVAILABLE` as appropriate. A partially downloaded resource is never activated.

## Cache policy

The cache should be content-addressed by SHA-256 whenever practical. A second installation of the same verified payload must reuse the cached object instead of downloading it again.

Metadata and payloads are separate so an index refresh does not require redownloading unchanged data.

## Network policy

The resource layer must not hard-code one transport implementation. The future network/download service provides byte streams; the resource layer performs manifest validation, dependency resolution, digest verification, staging, and activation.

No UI component may directly implement HTTP, TLS, or filesystem transaction logic.

## Offline behavior

Offline operation must use:

1. active installed resource;
2. valid cached resource;
3. smallest built-in fallback;
4. clear unavailable-resource UI.

An unavailable optional resource must not prevent the core desktop from booting.

## Base-image removal targets

The base image should avoid embedding:

- full language catalogs;
- large font families beyond the minimum fallback;
- wallpaper collections;
- optional sound packs;
- large icon packs;
- documentation bundles;
- optional applications;
- update/package payloads;
- duplicate resource variants.

Tiny bootstrap labels required before the resource service starts may remain compiled into the first-run UI.

## Integrity and trust

SHA-256 is an integrity identifier, not a trust root. The final distribution system must additionally authenticate repository metadata and/or resource signatures before treating untrusted remote content as trusted software. Package/update supply-chain requirements in `SB_OS_DESIGN.md` remain authoritative.

## Repository layout proposal

```text
SuiraBox-OS-Resources/
  manifest/
    manifest-v1.json
  locales/
  keyboards/
  fonts/
  themes/
  wallpapers/
  icons/
  sounds/
  apps/
  documents/
  signatures/
```

The resource repository is intentionally independent of the kernel source tree so resource updates do not require rebuilding the OS image.

## Current implementation status

`userspace/resource.[ch]` defines the resource contract and validates identifiers, paths, size limits, SHA-256 strings, dependencies, compatibility, and lifecycle transitions. Network transport, signature verification, cache storage, and transactional activation are subsequent implementation layers.
