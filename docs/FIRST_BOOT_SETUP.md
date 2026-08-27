# SuiraBox First Boot Setup

SuiraBox starts with a deliberately small first-boot setup wizard before the full kernel userspace stack is initialized.

## Current wizard

The early wizard is a VGA text-mode interface so it does not depend on the future desktop stack. It supports keyboard navigation with:

- Up / Down: select a setting
- Left / Right: change a setting
- Enter: continue
- Esc: keep the current defaults
- 1 / 2 / 3 / 4: select Japanese / English / Chinese / Spanish

The current settings are:

- Language
- Region / Time Zone (initial reference: Japan / JST)
- Keyboard (initial reference: Japanese)
- Network (configure later)
- Performance profile: Balanced / Performance / Low Resource / Custom

## Design goals

The screen is intentionally lightweight, English-first, and independent from storage/network drivers. It must not add the desktop GUI, browser, package manager, or background services merely to show setup.

The setup module has a bounded input loop. A machine without usable keyboard input therefore continues with safe defaults rather than blocking the entire boot forever.

## Persistence

The current implementation stores the selected configuration in kernel memory only. Persistent first-boot state will be connected to the writable filesystem once the storage/filesystem write path is ready. The persistent backend must use an explicit configuration file and atomic replacement so an interrupted setup cannot brick the installation.

Until persistent storage is implemented, the text wizard should be treated as an early-boot configuration prototype rather than the final desktop onboarding experience.

## Future desktop setup

The final GUI setup should reuse this same configuration schema instead of inventing a second format. It should present the same choices with a graphical interface and add optional detailed settings without making them mandatory.
