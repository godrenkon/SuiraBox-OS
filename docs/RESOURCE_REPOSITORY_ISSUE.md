# External Resource Repository TODO

Target repository: `godrenkon/SuiraBox-OS-Resources`

The target repository should be created outside the OS source tree. Its payloads must not be copied into the OS repository merely to make the desktop functional.

Creation checklist:

- create a separate public repository;
- add `manifest/manifest-v1.json` matching the SB Resource descriptor contract;
- publish stable/test channels;
- add SHA-256 metadata for every package;
- publish large locale/theme/wallpaper/icon/sound/application packages independently;
- keep Settings, Terminal, basic File Manager, fallback font/theme/background in `SuiraBox-OS` Core;
- configure the endpoint through Resource settings rather than compiling it into the kernel;
- never require the repository for first boot or offline Core operation.

Until this repository exists, the OS must treat Remote resources as unavailable and continue using Core fallbacks.
