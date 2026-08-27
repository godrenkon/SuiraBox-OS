# SuiraBox Build and Release Plan

## Release philosophy

SuiraBox should ship a small common core and add optional functionality as packages/features. Hardware-specific variants should be built from the same source tree rather than maintaining unrelated operating systems.

## Planned editions

- Desktop: graphical daily-use OS, current priority.
- Server/CUI: terminal-first edition, planned after the desktop edition reaches a usable milestone.

## Hardware expansion

Start with a conservative x86_64 baseline and add hardware-specific driver/profile packages over time. Future release profiles may target different NVIDIA GPU families, workstation/server hardware, and data-center configurations without forcing unused drivers into the minimal installation.

## Release artifacts

Future releases should provide clearly named artifacts for:

- Generic x86_64 hardware
- Hardware/profile-specific builds when genuinely necessary
- Server/CUI builds
- Development/testing builds

Every public release should include checksums, version/build identifiers, supported-hardware notes, known issues, installation instructions, recovery instructions, and reproducible build information where practical.

## Quality gates

A release should not be considered complete merely because the ISO builds. At minimum, CI should verify:

- Kernel/boot image format
- ISO creation
- QEMU boot
- Memory initialization
- Device discovery
- User-space startup
- Filesystem/package operations when available
- Error/panic paths
- First-boot GUI flow once the graphical shell exists

Optional hardware support should degrade gracefully: an unavailable device must not prevent the base OS from booting unless that device is genuinely required for the selected installation.
