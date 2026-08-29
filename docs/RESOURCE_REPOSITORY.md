# SuiraBox Resources Repository Contract

The external resource repository is a separate distribution boundary from the OS image. The intended repository name is `godrenkon/SuiraBox-Resources`; it is not assumed to exist until explicitly created and populated.

## Layout

```text
manifest/
  manifest-v1.json
  manifest-v1.sig
  keys/
    root.json
locales/
  ja-jp/
  en-us/
  zh-cn/
  es-es/
fonts/
themes/
wallpapers/
icons/
sounds/
apps/
```

Resource payloads are immutable and addressed by SHA-256. Human-facing paths are catalog paths; the OS cache uses the content hash as the object identity.

## Manifest rules

The manifest schema is versioned independently from OS API compatibility. A manifest entry contains an immutable resource ID, resource version, type, tier, exact compressed payload size, bounded expanded size, SHA-256, relative path, compression type, minimum OS API, and dependency IDs.

IDs and paths are lowercase-safe relative names. `.` and `..` path components are forbidden. Dependency count is bounded by the OS contract and cycles are rejected.

`manifest-v1.json` is the signed metadata object. `manifest-v1.sig` is a detached signature over the canonical UTF-8 manifest bytes. The verifier must validate the signature against a built-in trust anchor, reject unknown signing keys, reject malformed or unsupported schema versions, and enforce freshness/rollback policy before the parsed entries are exposed to the resource manager.

The Resource Manager v2 signed initialization contract passes both the exact canonical manifest bytes and the parsed entries to the verifier. A successful verification therefore binds the signature to the bytes from which the parsed manifest was produced instead of trusting an independently supplied in-memory entry array.

## Publication

A new payload must be uploaded before the manifest references it. The publisher computes SHA-256 and exact byte size from the final immutable payload, updates the manifest, signs the canonical manifest bytes, and publishes the manifest only after every referenced object is available.

A resource is never replaced in place under an existing SHA-256 object path. New content receives a new hash and a new version reference.

## Client behavior

```text
load trusted manifest bytes
  -> validate canonical bytes and signature
  -> parse manifest
  -> select resource by stable ID
  -> resolve dependencies
  -> check local hash-addressed cache
  -> download only missing objects
  -> stream to temporary storage
  -> verify exact size and SHA-256
  -> atomically publish cache object
  -> activate
```

Offline mode uses the Core baseline only. Missing optional/remote resources do not prevent normal desktop startup. Corrupt or partially downloaded cache objects are never activated.

## Delta and CDN compatibility

The repository contract intentionally does not require a particular transport. HTTP today and a CDN, mirror, range transport, or future package backend can implement the same logical fetch operation. Delta transfer is an optimization and must reconstruct the exact target SHA-256 before activation.

## First rollout

The first production pack should externalize the four currently supported UI locales. The OS image retains only the minimal fallback strings and glyph coverage required for first boot, recovery, language selection, and fatal/error presentation.
