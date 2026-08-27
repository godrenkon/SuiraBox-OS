# SB Package Manifest

The SB Store, CLI, updater, and future profile system use one machine-readable package manifest model.

## Required fields

```text
id
name
version
channel
architecture
size
sha256
license
publisher
```

## Optional fields

```text
description
category
dependencies
conflicts
provides
permissions
services
startup
install_path
update_policy
recovery
variants
```

## Example

```yaml
id: org.suirabox.minecraft
name: Minecraft Manager
version: 0.1.0
channel: stable
architecture: [x86_64]
size: 184320
sha256: <artifact-sha256>
license: <license>
publisher: SuiraBox Project
description: Minecraft installation and instance manager
dependencies:
  - org.suirabox.jvm
  - org.suirabox.network
permissions:
  - filesystem.games
  - network.outbound
services: []
startup: on-demand
install_path: /apps/minecraft-manager
update_policy: stable
recovery: transactional
variants:
  - id: desktop
  - id: server
```

## Design goals

The manifest is metadata, not the application binary. Catalog clients can fetch manifests without downloading payloads.

Artifacts are selected after compatibility and dependency resolution. A package can expose multiple architecture, GPU, or profile variants so an installation does not download a universal bundle.

The manifest must remain stable enough for third-party repositories and tools to implement independently.
