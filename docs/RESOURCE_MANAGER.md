# SB Resource Manager v1

The OS-side resource manager is intentionally independent of HTTP, FAT32 and any particular transport. It exposes a small callback ABI so networking and storage implementations can be replaced without changing the Desktop contract.

## Manifest v1

`docs/resource-manifest-v1.schema.json` is the machine-readable schema. JSON uses human-readable strings for `type`, `tier` and `compression`; the OS maps these to the compact `sb_resource_ref_t` representation.

`size` is the exact payload byte count delivered to the manager. When compression is not `none`, this is the compressed size. `expanded_size` is a hard metadata ceiling and must never exceed the core's 64 MiB resource limit.

Resource IDs are lowercase, stable logical identifiers such as `locale/ja-jp`. Repository paths are relative, lowercase-safe paths and reject `.` / `..` path components. SHA-256 is the immutable payload identity.

## Acquisition contract

```text
find ID
  -> validate complete manifest
  -> resolve dependencies depth-first
  -> builtin: activate shipped object
  -> cache hit: activate content-addressed object
  -> cache miss: begin temporary object
  -> stream payload into sink + SHA-256
  -> require exact payload size
  -> require SHA-256 equality
  -> atomic cache commit
  -> activate verified object
```

A failed transfer, size mismatch, hash mismatch or commit failure aborts the temporary transaction. A previously active object is not replaced by failed data.

Dependency cycles are rejected at acquisition time with a bounded depth, so malformed manifests cannot recurse without a limit.

## Cache contract

The logical cache identity is:

`/cache/suirabox/objects/sha256/<64 lowercase hex characters>`

Implementations may map this logical path onto another filesystem representation, but the content hash remains the identity. Multiple resource IDs may reference the same object without storing duplicate payloads.

## Security boundary

The manager verifies syntax, resource limits, ABI compatibility, path safety, dependency presence/cycles and SHA-256 before activation. It does not treat a remote payload as executable kernel code.

Manifest signatures, delta/range transport and decompression are intentionally separate layers. Until a verified implementation exists, compressed payloads are treated as opaque bytes and are not activated as decompressed data by the manager itself.

## First concrete resource

The first production resource should be `locale/ja-jp`, followed by the other selected UI locales. The Core retains only the minimal fallback strings/glyph coverage required for first boot, errors, recovery, network setup and language selection.
