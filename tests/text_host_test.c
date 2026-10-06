#include "text.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define WIDTH 64u
#define HEIGHT 64u
#define SENTINEL 0x12345678u
typedef struct {
    uint32_t before[8], pixels[WIDTH * HEIGHT], after[8];
    unsigned calls, fail_at;
} fixture_t;
static int check(int condition, const char *name) {
    if (!condition) { fprintf(stderr, "text test FAILED: %s\n", name); return 1; }
    return 0;
}
static void reset(fixture_t *f) {
    memset(f, 0, sizeof(*f));
    for (unsigned i = 0u; i < 8u; ++i) f->before[i] = f->after[i] = SENTINEL;
    for (unsigned i = 0u; i < WIDTH * HEIGHT; ++i) f->pixels[i] = SENTINEL;
}
static int emit(void *context, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *rgb) {
    fixture_t *f = context;
    ++f->calls;
    if (check(w && h && w * h <= 256u && x < WIDTH && y < HEIGHT &&
        w <= WIDTH - x && h <= HEIGHT - y, "bounded transport tile")) return -1;
    if (f->calls == f->fail_at) return -1;
    for (uint32_t row = 0u; row < h; ++row)
        memcpy(f->pixels + (y + row) * WIDTH + x, rgb + row * w, w * sizeof(*rgb));
    return 0;
}
static sb_text_renderer_t renderer(fixture_t *f, unsigned scale) {
    return (sb_text_renderer_t){.width = WIDTH, .height = HEIGHT, .foreground = 0xf2ae43u,
        .background = 0x0c1018u, .scale = scale, .emit = emit, .context = f};
}
int main(void) {
    fixture_t f, original;
    reset(&f);
    sb_text_renderer_t r = renderer(&f, 1u);
    if (check(sb_text_draw(&r, 2, 3, "A", 1u) == 0 && f.calls == 1u, "ASCII cell")) return 1;
    const unsigned rows[7] = {14u,17u,17u,31u,17u,17u,17u};
    for (unsigned y = 0u; y < HEIGHT; ++y) for (unsigned x = 0u; x < WIDTH; ++x) {
        uint32_t expected = SENTINEL;
        if (x >= 2u && x < 10u && y >= 3u && y < 11u) {
            const unsigned col = x - 2u, row = y - 3u;
            expected = row < 7u && col >= 1u && col <= 5u && (rows[row] & (1u << (5u-col))) ? r.foreground : r.background;
        }
        if (check(f.pixels[y * WIDTH + x] == expected, "exact A and opaque spacing")) return 1;
    }
    /* Signed clipping and 2x replication: compare with a fully visible cell,
     * checking every output pixel and both surrounding memory guards. */
    reset(&original); sb_text_renderer_t full = renderer(&original, 2u);
    if (check(sb_text_draw(&full, 0, 0, "A", 1u) == 0, "full scaled glyph")) return 1;
    reset(&f); r.scale = 2u;
    if (check(sb_text_draw(&r, -3, -5, "A", 1u) == 0 && f.calls == 1u, "negative clipping")) return 1;
    for (unsigned y = 0u; y < HEIGHT; ++y) for (unsigned x = 0u; x < WIDTH; ++x)
        if (check(f.pixels[y * WIDTH + x] == (x < 13u && y < 11u ? original.pixels[(y+5u)*WIDTH+x+3u] : SENTINEL),
                  "clipped pixel source offsets")) return 1;
    for (unsigned i = 0u; i < 8u; ++i)
        if (check(f.before[i] == SENTINEL && f.after[i] == SENTINEL, "no guard overwrite")) return 1;
    reset(&f);
    if (check(sb_text_draw(&r, 59, 61, "A", 1u) == 0 && f.calls == 1u &&
        f.pixels[61u*WIDTH+59u] == r.background && f.pixels[60u*WIDTH+59u] == SENTINEL,
        "right and bottom clipping")) return 1;
    reset(&f); r.scale = 1u;
    if (check(sb_text_draw(&r, 0, 0, "A\tB\rC\nD", 7u) == 0 && f.calls == 4u &&
        f.pixels[4] == r.foreground && f.pixels[32u+2u] == r.foreground &&
        f.pixels[8u*WIDTH+2u] == r.foreground && f.pixels[16] == SENTINEL,
        "tab stops, CR overwrite and newline origin")) return 1;
    reset(&f);
    if (check(sb_text_draw(&r, 0, 0, "\xe6\x97\xa5\xf0\x9f\x8e\xae\xc3\xa9", 9u) == 0 && f.calls == 3u,
        "one replacement per Unicode scalar")) return 1;
    for (unsigned y = 0u; y < 8u; ++y) for (unsigned x = 0u; x < 8u; ++x)
        if (check(f.pixels[y*WIDTH+x] == f.pixels[y*WIDTH+x+8u] &&
                  f.pixels[y*WIDTH+x] == f.pixels[y*WIDTH+x+16u], "consistent replacement glyph")) return 1;
    static const unsigned char bad[][6] = {
        {'A',0xc0,0xaf}, {'A',0xc1,0xbf}, {'A',0xe0,0x80,0x80}, {'A',0xed,0xa0,0x80},
        {'A',0xf4,0x90,0x80,0x80}, {'A',0xf5,0x80,0x80,0x80}, {'A',0xff}, {'A',0x80},
        {'A',0xe2,0x28,0xa1}, {'A',0xc2}, {'A',0xe2,0x82}, {'A',0xf0,0x9f,0x8e},
        {'A',0}, {'A',0x1b}, {'A',0x7f}, {'A',0xc2,0x80},
    };
    static const unsigned lengths[] = {3,3,4,4,5,5,2,2,4,2,3,4,2,2,2,3};
    for (unsigned i = 0u; i < sizeof(lengths)/sizeof(lengths[0]); ++i) {
        reset(&f);
        if (check(sb_text_draw(&r, 0, 0, (const char *)bad[i], lengths[i]) == SB_TEXT_INVALID && f.calls == 0u &&
            f.pixels[0] == SENTINEL, "malformed suffix rejects complete string before paint")) return 1;
    }
    reset(&f);
    /* Exact-length input is not NUL-terminated; no read of a trailing byte. */
    const char one[1] = {'A'};
    if (check(sb_text_draw(&r, 0, 0, one, sizeof(one)) == 0 && sb_text_draw(&r, 0, 0, 0, 0u) == 0,
        "explicit length and empty string")) return 1;
    reset(&f);
    for (unsigned i = 0u; i < 8u; ++i) {
        sb_text_renderer_t invalid = r;
        if (i == 0) invalid.width = 0;
        if (i == 1) invalid.height = 0;
        if (i == 2) invalid.width = UINT32_MAX;
        if (i == 3) invalid.height = UINT32_MAX;
        if (i == 4) invalid.scale = 0;
        if (i == 5) invalid.scale = 3;
        if (i == 6) invalid.emit = 0;
        if (check(sb_text_draw(i == 7 ? 0 : &invalid, 0, 0, one, 1u) == SB_TEXT_INVALID && f.calls == 0u,
            "invalid renderer draws nothing")) return 1;
    }
    if (check(sb_text_draw(&r, 0, 0, 0, 1u) == SB_TEXT_INVALID &&
        sb_text_draw(&r, 0, 0, one, SB_TEXT_MAX_BYTES+1u) == SB_TEXT_INVALID && f.calls == 0u,
        "invalid input length/pointer")) return 1;
    char long_text[SB_TEXT_MAX_BYTES]; memset(long_text, 'A', sizeof(long_text));
    if (check(sb_text_draw(&r, INT32_MIN, INT32_MIN, long_text, sizeof(long_text)) == 0 &&
        sb_text_draw(&r, INT32_MAX, INT32_MAX, long_text, sizeof(long_text)) == 0 && f.calls == 0u,
        "extreme signed origins stay offscreen without overflow")) return 1;
    reset(&f); f.fail_at = 2u;
    if (check(sb_text_draw(&r, 0, 0, "ABC", 3u) == SB_TEXT_EMIT_FAILED && f.calls == 2u &&
        f.pixels[0] == r.background && f.pixels[8] == SENTINEL && f.pixels[16] == SENTINEL,
        "callback error stops without later cells")) return 1;
    puts("UTF-8 bitmap text host test OK");
    return 0;
}
