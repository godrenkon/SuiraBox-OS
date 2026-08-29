# SB Desktop Resource Manifest Schema

The manifest is intentionally small. It describes optional resources; it is not a copy of the resources themselves.

```json
{
  "schema": 1,
  "channel": "stable",
  "resources": [
    {
      "id": "locale.ja-JP",
      "version": "1.0.0",
      "type": "locale",
      "size": 0,
      "sha256": "<64-hex-digest>",
      "path": "locales/ja-JP/locale.pack.zst",
      "compression": "zstd",
      "min_os": 1,
      "dependencies": [],
      "publisher": "suirabox",
      "license": "<license-id>"
    }
  ]
}
```

## Rules

- `id` is stable and unique within a resource namespace.
- `version` identifies the resource revision, not the OS release.
- `type` determines the consumer and validation rules.
- `size` is the exact payload size the transport is expected to deliver.
- `sha256` is the immutable content identity used by the local cache.
- `path` is resolved relative to the trusted Resource Repository root; path traversal is forbidden.
- `compression` describes transport/storage encoding. The decompressed representation must also pass type-specific validation.
- `min_os` is the minimum OS resource ABI version.
- `dependencies` lists resource IDs that are required for activation.
- `publisher` and `license` provide provenance information.

## Core/Resource separation

The manifest must never describe Core components as downloadable requirements. Core must remain bootable and usable when the manifest is unavailable.

A manifest may describe:

- language packs;
- additional fonts;
- themes and icon packs;
- wallpapers;
- sound packs;
- optional applications;
- large documentation datasets.

A manifest must not be required for:

- kernel boot;
- basic GUI;
- recovery terminal;
- storage recovery;
- security/integrity primitives;
- network initialization needed to reach the Resource service.

## Verification

The client accepts a resource only after:

1. manifest/channel policy validation;
2. compatibility validation;
3. exact byte count validation;
4. SHA-256 validation;
5. type-specific validation;
6. atomic cache activation.

For executable resources, a future signed-package requirement is mandatory; SHA-256 alone is not an authorization mechanism.
