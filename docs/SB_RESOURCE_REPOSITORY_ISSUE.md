# External resource repository bootstrap

Create the separate GitHub repository:

`godrenkon/SuiraBox-OS-Resources`

Required initial contents:

```text
manifest/manifest-v1.json
locales/ja-jp/locale.pack.zst
locales/en-us/locale.pack.zst
locales/zh-cn/locale.pack.zst
locales/es-es/locale.pack.zst
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

The manifest must be metadata-only and follow `docs/SB_RESOURCE_MANIFEST_V1.md` in the OS repository.

Do not copy OS source, kernel binaries, or mandatory boot assets into this repository. Resource payloads must be independently versioned so the OS image does not need rebuilding when resources change.

The resource repository should publish immutable versioned payloads and authenticated metadata. A stable `main` manifest may point to current compatible versions, while payload paths remain content-addressed or version-specific.

The OS currently references the repository through:

`https://raw.githubusercontent.com/godrenkon/SuiraBox-OS-Resources/main/manifest/manifest-v1.json`

This repository cannot currently be created automatically because the available GitHub integration exposes repository listing/search and file/commit operations but not a repository-creation endpoint.
