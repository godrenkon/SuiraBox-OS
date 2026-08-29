# External Resource Repository Setup

Planned repository name:

`godrenkon/SuiraBox-OS-Resources`

The base OS repository must not contain the payloads stored there.

## Required layout

```text
manifest/
  stable.json
locales/
  ja-JP/
  en-US/
  zh-CN/
  es-ES/
themes/
icons/
wallpapers/
fonts/
sounds/
apps/
```

## Core compatibility

The OS keeps only a tiny fallback set and the code required to retrieve and verify Optional Resources.

## Repository creation

The currently available GitHub integration can read and write repository contents but does not expose a repository-creation mutation. Therefore creation of this separate repository must be performed outside the current integration before the download endpoint becomes active.

The intended endpoint is recorded in the Core Resource contract so the repository can be created independently without changing the OS architecture.
