# Resource Decision Matrix

| Item | Classification | Core requirement |
|---|---|---|
| Kernel / boot | Core | Always required |
| Memory / process / VM | Core | Always required |
| Display / input | Core | Required for GUI |
| Minimal compositor / window manager | Core | Required for usable GUI |
| Minimal Settings / recovery UI | Core | Required for offline recovery |
| Minimal Terminal / recovery CLI | Core | Required for diagnostics |
| Storage / VFS / filesystem | Core | Required for local operation |
| Network transport needed by Resource client | Core | Required to obtain optional data |
| Resource verification / cache | Core | Required to use optional data safely |
| Tiny fallback glyphs / strings | Core | Required before resource download |
| Default tiny theme/chrome | Core | Required to make Core usable |
| Full language packs | Optional Resource | User-selected |
| Additional fonts | Optional Resource | User-selected and potentially large |
| Extra themes | Optional Resource | User-selected |
| Large icon packs | Optional Resource | User-selected |
| Wallpapers | Optional Resource | User-selected |
| Sound packs | Optional Resource | User-selected |
| Large help/tutorial data | Optional Resource | Nonessential |
| Optional applications | Optional Resource | User-selected |
| Advanced versions of Core tools | Optional Resource | Not required for recovery |
| Decoded image cache | Generated Cache | Reproducible |
| Glyph atlas cache | Generated Cache | Reproducible |

## Override rule

A small item may remain in Core even when it could theoretically be downloaded. A component must not be externalized solely to make the ISO smaller.

Conversely, a large component must not be embedded merely because it is convenient during development if the final product can operate correctly without it.

## Required offline test

For every candidate Optional Resource:

1. remove it;
2. disconnect the network;
3. boot the base OS;
4. open basic Settings and recovery Terminal;
5. verify that the OS remains usable.

Only then is externalization acceptable.
