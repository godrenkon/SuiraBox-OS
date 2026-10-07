# Desktop shell foundation

Normal graphical boot starts the PID 1 desktop after the existing userspace
storage, IPC and concurrent-process lifecycle checks. It uses the existing
DISPLAY_INFO, DISPLAY_PRESENT and KEY_EVENT_READ ABI; no physical display
address or kernel pointer is exposed. Headless boot continues without a shell.

## Controls

| Key | Action |
| --- | --- |
| F1 / Escape | Home |
| F2 | Files |
| F3 | Settings |
| Tab / Shift+Tab | Next / previous view |
| B / D in Files | Read `/boot` / `/disk` |
| R in Files | Reload the current directory |
| Up / Down in Files | Select a visible entry |
| Enter / Backspace in Files | Open / parent directory (close a preview first) |

Only an initial key-down activates an action. Releases, auto-repeat and
Ctrl/Alt/Meta chords do not navigate. Caps Lock does not affect these physical
key bindings. Input overflow displays a recovery notice; the driver maintains
the current modifier snapshot and the shell waits for fresh key presses.

Home shows the actual development status. Settings reports the display
resolution, PS/2 US keyboard availability and ASCII font limitations; it does
not yet change or persist configuration.

## Read-only Files view

The [Files browser](FILES_BROWSER.md) now supports bounded child/parent directory
navigation and read-only file previews. Escape closes a preview before returning
Home. Files reads DIRECTORY_OPEN / DIRECTORY_READ from the real VFS, showing type,
name and byte size. It defaults to `/boot`; the selected volume persists when
switching views. `/disk` is the mounted FAT32 volume. A missing/unmounted volume,
read error, invalid entry or close error becomes a recoverable inline message.
B, D and R permit another attempt. No create, write, rename or sync is issued.

A snapshot holds at most 12 rows and reads one extra entry to detect truncation.
Every successful open gets exactly one close attempt, including EOF, errors and
truncation. Failed snapshots discard partial rows. Metadata is validated before
rendering. Names display at most 24 ASCII bytes; nonprintable/non-ASCII bytes
become `?`, and longer names end with `~`. Control bytes cannot move the text
cursor. Sizes cover the full uint64 range. This is not a Unicode file browser.

The renderer clears previous content before drawing each view. Solid fills are
clipped and use at most 16x16-pixel tiles; text uses the existing bitmap emitter.
640x480 is the minimum usable layout. Smaller displays get a clipped size notice.
Display syscall failures abort the frame; a serial frame marker is emitted only
after all rendering completes. These markers let CI capture stable frames.

## Verification and limits

`make check` includes separate model and view host tests: navigation, modifiers,
repeat/release suppression, overflow, exact-boundary truncation, malformed
entries, handle cleanup on all exits, max-size formatting, sanitized names,
bounded transport, small displays and emitter failure. CI boots the ordinary
shell without a special proof compile flag, injects QMP keys through PS/2 IRQ1,
and captures ten completed frames. An independent Python checker compares every
RGB pixel, using the built ELF sizes and fixture size for the actual VFS rows.
Disk SHA-256 must remain unchanged. Existing storage, graphics, text, keyboard
editing and cross-process IPC proofs remain separate regression checks.

This is a single PID 1 shell. Display/input service separation, compositor,
windows, mouse, scrolling, file editing/launching, CJK
fonts/IME, first-run language selection, settings persistence, networking, JVM
and the Minecraft launcher remain unfinished. Roadmap 17-3, 17-5, 17-6 and 11-5
are still partial and are not marked complete.
