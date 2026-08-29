# SB Desktop Resource Architecture

## Purpose

SB Desktop is intentionally split into three resource tiers so the OS image stays small without turning the initial installation into an empty shell.

## Tier 0: Core (always built-in)

Core resources are required for the operating system to function or to provide the promised baseline desktop experience. They MUST be present in the installation image and MUST NOT require a network connection.

Examples:

- kernel, boot path, memory management, scheduler and process primitives;
- framebuffer/display, keyboard and mouse input;
- compositor, window manager and Desktop Shell;
- first-run Settings UI and the language selector itself;
- the default system font/glyph set needed to render the UI;
- the default fallback theme and minimal wallpaper/background;
- filesystem and configuration recovery logic;
- the minimal terminal/CLI required by the GUI edition;
- resource-manager code, manifest parser and integrity verification;
- recovery UI and offline error handling.

Core must be sufficient to boot, reach the desktop, open Settings, change fundamental settings, use the terminal, and recover from missing optional/remote assets.

## Tier 1: Optional (locally installable)

Optional resources are not necessary for the baseline OS and may be disabled to reduce RAM/storage use. They are packaged separately from the kernel/runtime when that gives a measurable size or maintenance benefit, but the product must be able to install them locally without requiring a remote fetch.

Examples:

- additional themes that are small enough to ship locally;
- extra language packs whose assets exceed the core glyph set;
- optional system utilities;
- extra fonts;
- additional shell providers;
- optional accessibility components.

The Settings UI controls whether these components are enabled. Disabling an optional component must release or avoid its runtime resources where practical; it must not uninstall the core recovery path.

## Tier 2: Remote / On-demand

Remote resources are selected by the user or are explicitly requested by an installed feature. They live outside the OS repository and are downloaded only when needed.

Examples:

- large language packs;
- high-resolution wallpapers and wallpaper collections;
- large theme packs;
- icon packs;
- sounds and notification packs;
- optional applications and application bundles;
- large documentation/help datasets;
- game-specific integrations, including Minecraft-related assets;
- development toolchains and other large packages.

Remote resources MUST NOT be required to reach a usable desktop.

## Resource repository policy

Remote content is intended to live in a separate repository from `SuiraBox-OS`. The OS repository contains the resource client, manifest schema, version policy, cache policy and a tiny built-in fallback set; it does not contain large asset collections.

Each remote resource is addressed by a stable resource ID plus version and cryptographic digest. The client downloads only the selected object (or a delta where supported), verifies its digest before activation, stores it in a content-addressed cache, and reuses an already verified local copy.

A failed, interrupted or unavailable download must leave the previously active resource intact. An invalid digest must never be activated.

## Default user experience

The first boot must not become a package manager installation wizard. Core desktop functionality is already available.

When a user opens a setting that needs an optional/remote asset, the UI should show the choice and download state in-place. The user can continue using the rest of the desktop while an optional resource is unavailable or downloading, subject to the feature's own requirements.

## Configuration model

The configuration system distinguishes:

- `builtin`: mandatory and immutable from the resource manager's perspective;
- `optional`: enabled/disabled by user configuration;
- `remote`: selected, cached and activated by user configuration.

A configuration entry may reference a resource ID but must never assume that the resource is physically present. Capability checks precede activation.

## Size and performance rules

1. Never ship an asset merely because a feature exists. Ship only what is needed by Core.
2. Never embed the same asset into multiple binaries.
3. Prefer shared immutable assets over per-process copies.
4. Prefer compressed remote packages for large collections and decompress into the cache only when required.
5. Do not keep inactive optional resources resident in RAM.
6. Download on demand; never prefetch large resources without an explicit policy.
7. Cache verified remote resources and reuse them across reboots.
8. Garbage-collect unused cached resources according to storage pressure and user pinning.
9. Keep the resource manager small enough that its own footprint does not defeat the purpose of this architecture.
10. Core must contain a tiny fallback for every user-visible surface that otherwise depends on optional/remote resources.

## Security and integrity

Remote resources are untrusted input. The resource manager must validate:

- resource ID and requested version;
- package length limits;
- manifest syntax;
- cryptographic digest;
- package type and declared capabilities;
- cache path safety;
- decompression limits before allocation;
- compatibility with the running SB Desktop resource ABI.

Remote resources are data/modules, not an opportunity to replace the kernel or bypass the process/security model.

## Future repository layout

The external resource repository should be independently versioned and should not be required to build the kernel. A future layout should group content by stable resource ID instead of by UI screen, for example:

```text
manifest/
  index.json
  channels/
assets/
  locale/
  theme/
  wallpaper/
  icons/
  sounds/
apps/
  settings/
  files/
  terminal/
```

The exact external repository name is intentionally not hard-coded here until that repository is actually created. The operating system must treat the repository endpoint as configuration, not as a compile-time dependency.
