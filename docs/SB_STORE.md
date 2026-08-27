# SB Store and Package System

SB OS should be usable without requiring users to type package-management commands.

## User-facing model

The default desktop includes an SB Store and an SB Settings application. The terminal remains available for advanced users and server editions.

A user selects an application or feature, reviews its size/dependencies/permissions, and presses Install. The Store resolves metadata first, then downloads only the required artifacts.

## Storage model

- `/system` — minimal operating-system components
- `/runtime` — shared runtimes and platform components
- `/apps` — installed applications
- `/games` — game installations and game-specific components
- `/data` — user data
- `/cache` — disposable package and application cache

Optional packages must not silently move user data into system-owned locations.

## Package lifecycle

1. Resolve package metadata.
2. Verify repository and package signatures.
3. Resolve dependencies.
4. Show total download/storage impact.
5. Download required artifacts, using cache/resume/delta mechanisms where available.
6. Verify hashes.
7. Stage files.
8. Activate the package and its declared services.
9. Record an uninstall/rollback transaction.

## Repository design

Repository metadata should be small, signed, cacheable, and independently downloadable from package payloads.

A repository may provide multiple artifact variants for:

- x86_64 / future architectures
- GPU backend
- desktop/server/data-center profile
- debug/release builds
- language/runtime variants

The client chooses the smallest compatible artifact set rather than downloading a universal bundle.

## Optional components

Examples include browsers, IDEs, media tools, JVM builds, Minecraft launchers, Fabric/NeoForge tooling, Minecraft server tooling, graphics backends, and specialized drivers.

The minimal installation should not carry large optional payloads merely so they are available.

## Community repositories

The format should support Official, Community, and Local repositories. Repository trust, signatures, maintainer identity, package permissions, and compatibility must be visible in the UI.

## CLI compatibility

The same package API exposed by the GUI is available to the `sb` CLI, for example:

```text
sb search minecraft
sb install minecraft
sb remove minecraft
sb update
```

The CLI is an interface to the same package transaction engine, not a separate implementation.
