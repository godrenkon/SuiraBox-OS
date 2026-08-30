# SB Resource Prefetch Policy

## Purpose

SB Desktop must not force users to perform a long sequence of small resource downloads during normal use.

The Resource Manager therefore supports two complementary acquisition modes:

1. **Initial prefetch**: during installation or first-run setup, acquire the resources already known to be required by the selected system profile and user choices.
2. **On-demand acquisition**: later acquire a resource only when it becomes required and is not already present in the verified cache.

This preserves the small base image while avoiding unnecessary repeated downloads during normal operation.

## Initial prefetch boundary

The installer/first-run planner must build an explicit prefetch set from:

- the selected UI locale;
- resources required by the selected Desktop profile;
- resources required by selected accessibility features;
- resources required by explicitly selected optional applications/components;
- dependencies of all of the above.

The planner must **not** prefetch resources merely because they exist in a repository manifest.

Large optional collections such as unrelated wallpapers, themes, icons, sounds, extra locales, documentation, codecs, and Minecraft-specific payloads remain on-demand unless the user has selected them or the active profile explicitly requires them.

## Execution contract

The prefetch operation is a bounded set acquisition over stable resource IDs:

```text
selected resource IDs
    ↓
deduplicate IDs
    ↓
resolve dependencies
    ↓
cache-hit check
    ↓
fetch missing objects
    ↓
exact size verification
    ↓
SHA-256 verification
    ↓
atomic cache commit
    ↓
activate all verified resources
```

The operation must reuse the normal Resource Manager acquisition path rather than introducing a second download or verification implementation.

## Failure semantics

A failed resource must never replace an existing verified cached object.

If a required initial-prefetch resource cannot be acquired, first-run completion must not be committed as fully ready. The installer may retry or fall back only when the active system profile explicitly defines a valid fallback resource.

Already verified cache objects remain usable after a failed prefetch attempt.

## Idempotence

Running initial prefetch more than once must be safe:

- already cached content is reused;
- identical content is not downloaded again;
- duplicate resource IDs do not create duplicate cache objects;
- dependency acquisition is bounded by the normal manager depth limit;
- a successful second run produces the same active resource set as the first run.

## Network and offline behavior

Initial prefetch should be performed while a usable network path is available when the selected profile requires remote resources.

If the system can satisfy the entire selected profile from built-in resources and existing verified cache objects, no network transfer is required.

When required remote resources are unavailable and no valid cached/fallback copy exists, setup must report a recoverable error rather than silently continuing with an incomplete Desktop.

## Performance

Prefetching should batch the planning stage and avoid repeated manifest parsing, dependency traversal, or transport setup for the same resource set where the underlying transport supports batching.

The cache remains content-addressed so resources shared by multiple profiles, applications, or IDs are stored only once.

## Security

Prefetch does not weaken verification rules. Every remote payload must still satisfy the existing manifest compatibility checks, exact payload-size check, SHA-256 integrity check, and atomic cache-commit rules before activation.

Remote resources are never treated as executable kernel code merely because they were prefetched.

## Implementation target

The next Resource Manager API extension should provide a bounded operation equivalent to:

```text
prefetch(resource_ids[], resource_count)
```

It should return success only when every requested resource and its dependencies are usable or verified in cache.

The existing single-resource acquisition primitive remains the underlying operation for on-demand use and for each member of the prefetch set.
