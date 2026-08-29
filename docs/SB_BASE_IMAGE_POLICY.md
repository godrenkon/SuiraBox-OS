# SB Desktop Base Image Policy

The base image is a minimal bootable runtime, not a full software bundle.

## Development and release profiles

Development CI intentionally keeps boot diagnostics and self-tests enabled so regressions remain visible.

Release builds use `scripts/build_release_iso.sh` and the dedicated `release-iso` target. The release kernel uses `kernel/release_entry.c` instead of the diagnostic development `kernel/kernel.c`, so boot self-tests, verbose serial diagnostics, and the test-only storage object are not linked into the distributed kernel.

Both profiles use section-level compilation and linker garbage collection so unreachable code/data does not consume the final image.

## Allowed runtime payload

The release ISO is allowed to contain only:

```text
/boot/suirabox.elf
/boot/sb-desktop.elf
/boot/grub/grub.cfg
```

Build-only test artifacts may exist under `build/`, but are never copied into the release ISO.

## Explicitly externalized

The following must not be embedded in the release ISO unless a future architecture decision marks a specific item as mandatory for boot/recovery:

- full locale catalogs;
- keyboard layout packs;
- large font families;
- themes;
- wallpaper collections;
- icon packs;
- UI sound packs;
- documentation bundles;
- optional applications;
- application databases and package payloads;
- update payloads;
- download caches;
- duplicate architecture/resource variants.

## Minimal fallback rule

A small built-in fallback is permitted when it is required to boot or configure the machine before networking/resource services are available. Such fallback must be deliberately small and must not become a second copy of a complete resource package.

## Performance rule

Externalization must not impose a permanent RAM tax. Resource metadata should be lazy-loaded, content-addressed caches should be reused, and only active resources should be mapped into memory.

The base runtime should not allocate or retain optional resource payloads until the corresponding feature is selected.

## Repository rule

Non-boot-critical resources are maintained in the separate `godrenkon/SuiraBox-OS-Resources` repository. The OS source repository contains the resource ABI/policy and metadata contracts, not the payload collection.

## CI enforcement

`scripts/check_base_image.sh` verifies the generated ISO staging tree against the allowlist. `scripts/check_externalized_payloads.sh` rejects vendored binary resource payloads and oversized tracked source files. The two policies prevent optional resources from silently migrating back into the base image.
