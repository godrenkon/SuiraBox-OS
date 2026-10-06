#include "text.h"
#include "font_bitmap.h"

/* Decode without reading beyond the explicitly supplied byte range. */
static int scalar(const unsigned char *s, size_t length, size_t *offset, uint32_t *value) {
    const unsigned char first = s[(*offset)++];
    if (first < 0x80u) { *value = first; return 0; }
    uint32_t code, minimum;
    unsigned tail;
    if (first >= 0xc2u && first <= 0xdfu) { code = first & 31u; minimum = 0x80u; tail = 1u; }
    else if (first >= 0xe0u && first <= 0xefu) { code = first & 15u; minimum = 0x800u; tail = 2u; }
    else if (first >= 0xf0u && first <= 0xf4u) { code = first & 7u; minimum = 0x10000u; tail = 3u; }
    else return SB_TEXT_INVALID;
    if (tail > length - *offset) return SB_TEXT_INVALID;
    for (unsigned i = 0u; i < tail; ++i) {
        const unsigned char next = s[(*offset)++];
        if ((next & 0xc0u) != 0x80u) return SB_TEXT_INVALID;
        code = (code << 6u) | (next & 63u);
    }
    if (code < minimum || code > 0x10ffffu || (code >= 0xd800u && code <= 0xdfffu))
        return SB_TEXT_INVALID;
    *value = code;
    return 0;
}
static int allowed(uint32_t code) {
    return code == '\n' || code == '\r' || code == '\t' ||
           (code >= 32u && code != 127u && !(code >= 128u && code <= 159u));
}
static int glyph(const sb_text_renderer_t *r, int64_t x, int64_t y, uint32_t code) {
    const int64_t cell = 8u * r->scale;
    if (x >= r->width || y >= r->height || x + cell <= 0 || y + cell <= 0) return 0;
    const int64_t left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
    const int64_t right = x + cell > r->width ? r->width : x + cell;
    const int64_t bottom = y + cell > r->height ? r->height : y + cell;
    if (code > 126u) code = '?';
    const uint8_t *rows = sb_font5x7[code - 32u];
    uint32_t pixels[256];
    size_t index = 0u;
    for (int64_t dy = top; dy < bottom; ++dy) {
        const unsigned row = (unsigned)(dy - y) / r->scale;
        for (int64_t dx = left; dx < right; ++dx) {
            const unsigned column = (unsigned)(dx - x) / r->scale;
            const int ink = row < 7u && column >= 1u && column <= 5u &&
                            (rows[row] & (1u << (5u - column))) != 0u;
            pixels[index++] = ink ? r->foreground : r->background;
        }
    }
    return r->emit(r->context, (uint32_t)left, (uint32_t)top,
                    (uint32_t)(right - left), (uint32_t)(bottom - top), pixels) == 0 ? 0 : SB_TEXT_EMIT_FAILED;
}
int sb_text_draw(const sb_text_renderer_t *r, int32_t x, int32_t y, const char *utf8, size_t length) {
    if (r == 0 || r->emit == 0 || r->width == 0u || r->height == 0u ||
        r->width > INT32_MAX || r->height > INT32_MAX || (r->scale != 1u && r->scale != 2u) ||
        length > SB_TEXT_MAX_BYTES || (length != 0u && utf8 == 0)) return SB_TEXT_INVALID;
    const unsigned char *bytes = (const unsigned char *)utf8;
    size_t offset = 0u;
    uint32_t code;
    while (offset < length) {
        if (scalar(bytes, length, &offset, &code) != 0 || !allowed(code)) return SB_TEXT_INVALID;
    }
    int64_t column = 0, line = 0;
    offset = 0u;
    while (offset < length) {
        (void)scalar(bytes, length, &offset, &code); /* validated; input must remain stable during draw */
        if (code == '\n') { column = 0; ++line; }
        else if (code == '\r') column = 0;
        else if (code == '\t') column = (column / 4 + 1) * 4;
        else {
            const int result = glyph(r, (int64_t)x + column * 8u * r->scale,
                                      (int64_t)y + line * 8u * r->scale, code);
            if (result != 0) return result;
            ++column;
        }
    }
    return 0;
}
