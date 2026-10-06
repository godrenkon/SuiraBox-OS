#ifndef SB_USERSPACE_TEXT_H
#define SB_USERSPACE_TEXT_H
#include <stddef.h>
#include <stdint.h>

#define SB_TEXT_MAX_BYTES 4096u
#define SB_TEXT_INVALID (-1)
#define SB_TEXT_EMIT_FAILED (-2)

typedef int (*sb_text_emit_t)(void *context, uint32_t x, uint32_t y,
                              uint32_t width, uint32_t height, const uint32_t *rgb);
typedef struct {
    uint32_t width, height, foreground, background;
    uint32_t scale; /* 1 or 2: opaque 8x8 or 16x16 cells, at most 256 pixels. */
    sb_text_emit_t emit;
    void *context;
} sb_text_renderer_t;

/* Length-delimited UTF-8; printable ASCII uses the built-in original 5x7 font.
 * Other Unicode scalars use one '?' cell. LF starts a new line at x; CR resets
 * the column; TAB advances to the next four-cell stop. No automatic wrapping.
 * Clip against the display, including negative origins. Validate the entire
 * input before emitting; malformed UTF-8/controls/configuration draw nothing.
 * A callback failure stops immediately and may leave earlier cells visible.
 * The callback consumes/copies its temporary RGB buffer before returning.
 */
int sb_text_draw(const sb_text_renderer_t *renderer, int32_t x, int32_t y,
                 const char *utf8, size_t length);
#endif
