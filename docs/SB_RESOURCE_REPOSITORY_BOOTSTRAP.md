# `godrenkon/SuiraBox-OS-Resources` Bootstrap Checklist

The separate resource repository must be created as:

`godrenkon/SuiraBox-OS-Resources`

## Required root layout

```text
manifest/manifest-v1.json
locales/
keyboards/
fonts/
themes/
wallpapers/
icons/
sounds/
apps/
documents/
signatures/
```

## Initial policy

Only non-boot-critical assets belong here. The repository must not contain the OS kernel, bootloader, release ISO, source snapshot, or duplicated test payloads.

Payloads should be immutable once published, independently versioned, SHA-256 identified, and accompanied by authenticated metadata/signatures before the OS treats them as trusted software.

## First resources to migrate

1. Japanese and English locale packs.
2. Additional locale packs.
3. Optional multilingual fonts.
4. Optional themes.
5. Wallpapers.
6. Icons.
7. UI sounds.
8. Optional applications.
9. Documentation/help bundles.

## OS integration contract

The OS already defines the external manifest endpoint and Resource Registry. The resource downloader must implement transport separately from validation, dependency resolution, caching, and activation.

The base image must contain only tiny bootstrap fallbacks needed before networking or the resource service is available.
