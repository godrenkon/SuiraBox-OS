# Userspace bitmap text foundation

`userspace/text.h` exposes `sb_text_draw(renderer, x, y, utf8, length)` without a
heap, C runtime, native framebuffer pointer or new kernel syscall. An emit
callback consumes each tightly packed `0x00RRGGBB` tile synchronously. The boot
adapter submits it through DISPLAY_PRESENT; only bootstrap init owns the display
under the current kernel policy. Normal boots show the OS title and startup
status. Headless boots skip the display without failing the process lifecycle.

## Font and layout contract

- The original `userspace/font_bitmap.h` contains all 95 printable ASCII glyphs,
  including lowercase, digits and punctuation, in 5x7 artwork inside 8x8 cells.
- Scale 1 or 2 produces opaque 8x8 or 16x16 RGB cells. Foreground and background
  come from the caller. A tile always has at most 256 pixels.
- Input is explicitly length-delimited, at most 4096 bytes. No terminating NUL is
  required. An empty range may have a null pointer.
- Decode UTF-8 scalars strictly: reject truncated/stray continuations, overlong
  forms, surrogates and values above U+10FFFF. Embedded NUL and C0/C1 controls are
  rejected except LF, CR and TAB. Validate the complete input before any emit.
- A non-ASCII scalar uses one `?` cell, regardless of its encoded byte length.
  This includes Japanese, accented Latin and emoji; this is not CJK rendering,
  localization, shaping, bidirectional text or grapheme clustering.
- LF advances one cell row and resets the column to the supplied x origin; CR
  only resets the column; TAB advances to the next four-cell stop. No wrapping
  or scrolling occurs. Offscreen cells are skipped; partially visible cells are
  clipped at all four edges, including negative signed origins.
- Invalid input/configuration returns `SB_TEXT_INVALID` without painting. An emit
  failure returns `SB_TEXT_EMIT_FAILED`, stops further cells, and may leave
  previously emitted cells visible. Callers must keep input stable during draw.
  The callback must copy/consume its temporary pixel buffer before returning.

## Build and verification

Userspace C uses the large code model because the ELF lives at 0x8000000000.
SIMD, red-zone, unwind tables and implicit runtime dependencies are disabled.
The assembly-to-C bridge aligns the stack to the SysV convention; int 0x80 keeps
the existing syscall ABI. Display proof mode is tracked by the build stamp and
its C proof symbol is removed when the mode is disabled.

`make host-text-test` checks exact glyph pixels, opaque spacing, signed clipping,
scaling, tabs/CR/LF, fallback scalars, malformed suffix preflight, explicit length,
extreme origins and callback failures. QEMU CI captures both normal boot text and
the DISPLAY_PROOF fixture through QMP. The checker shares only the font artwork
with the guest and computes layout/rasterization independently, comparing every
RGB pixel. The fixture includes malformed UTF-8 directed at a protected header
pixel, so partial prefix drawing fails the screenshot comparison. Existing IPC
and storage proofs continue, and both capture-only fixture disks retain their
SHA-256. PNG, PPM and serial logs are retained as CI artifacts.

Next steps remain input events, a compositor/window service, font loading and
Unicode/CJK text. The proof labels and Continue rectangle are static; they are
not a working settings screen or language-selection dialog.
