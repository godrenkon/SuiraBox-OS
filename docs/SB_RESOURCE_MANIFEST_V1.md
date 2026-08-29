# SB Resource Manifest v1

The manifest is intentionally metadata-only. Payloads are never copied into `SuiraBox-OS`.

Example entry:

```json
{
  "schema": 1,
  "id": "locale/ja-jp",
  "type": "locale",
  "version": 1,
  "min_os_api": 1,
  "path": "locales/ja-jp/locale.pack.zst",
  "compressed_size": 123456,
  "expanded_size": 456789,
  "sha256": "64-lowercase-hex-characters",
  "dependencies": ["core/text"]
}
```

## Rules

`id` and `path` are relative identifiers only. They must not contain `..`, absolute paths, empty path segments, or unsupported characters.

`compressed_size` and `expanded_size` are hard limits used before allocation/decompression. The client rejects values above the configured maximum.

`sha256` must be exactly 64 hexadecimal characters followed by a NUL terminator in the in-memory representation.

`dependencies` are stable resource IDs and are bounded to the maximum dependency count supported by the OS ABI.

A resource is installable only when its OS API requirement is satisfied and every dependency resolves to a compatible verified resource.

## Versioning

Manifest schema versions are independent from resource versions. A schema migration must be implemented before a newer schema is accepted.

Resource updates are content-addressable. Changing payload content requires a new digest and should normally produce a new resource version.

## Security

Manifest metadata is untrusted until repository metadata authentication/signature verification is implemented. SHA-256 verifies content identity/integrity; it does not establish publisher trust.
