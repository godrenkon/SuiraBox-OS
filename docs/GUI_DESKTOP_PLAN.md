# SuiraBox Desktop GUI Plan

## Scope

The current target is the graphical desktop edition. The CUI/server edition is a separate future product and must not add unnecessary GUI components to the desktop core.

## First visible experience

The current implemented foundation is a validated boot RGB surface and bounded
DISPLAY_INFO/DISPLAY_PRESENT transport from init userspace. QEMU capture verifies
the actual screen after ring-3 rendering and rejection tests. The colored proof
pattern is a transport fixture. A userspace bitmap text layer now renders all
printable ASCII, strict UTF-8 fallback, tabs/newlines and clipped cells through
the same transport; normal boot shows the title and startup status. See
[text rendering](TEXT_RENDERING.md). The static proof labels do not implement a
desktop or localization dialog. The first PS/2 IRQ keyboard path now feeds
init's nonblocking input API. Normal boot starts a single-init Home/Files/Settings
shell with keyboard navigation, real `/boot` and `/disk` directory traversal
with bounded-memory paging and read-only file previews;
see [desktop shell](DESKTOP_SHELL.md) and [keyboard input](KEYBOARD_INPUT.md).
The line editor remains a separate IRQ regression fixture. Compositor, windows,
mouse/input routing, settings persistence and CJK fonts remain future steps.

On a fresh installation:

1. Firmware/bootloader starts the kernel.
2. Kernel and essential userspace services initialize.
3. The desktop is displayed.
4. A compact first-boot language dialog appears over the desktop.
5. The user selects a language from a dropdown and presses Continue.
6. The selected language is persisted atomically.
7. The desktop continues using the selected locale.

On later boots, a valid persisted configuration skips the dialog and enters the desktop directly.

## Initial languages

- Japanese
- English
- Chinese
- Spanish

English is an acceptable temporary fallback before the first selection.

## GUI principles

- Keep the first-boot UI small and fast.
- Do not use the serial console or terminal for normal GUI setup.
- Do not preload optional applications merely to display the desktop.
- Use the same localization service for Settings, notifications, errors, installer/store UI, and applications.
- Missing or corrupt configuration must safely return to first-boot setup.
- Language can later be changed from Settings.

## Error UX

Normal application/driver failures must remain recoverable and should use ordinary notifications or dialogs. BSOD/RSOD are reserved for kernel-level unrecoverable conditions and are not part of normal first boot.

## Architecture boundary

The desktop shell should depend on stable OS services rather than hardware-specific drivers directly. Hardware support belongs below the device/driver abstraction so the desktop can remain portable across PC configurations.
