#include "display_surface.h"

static uint32_t mask(uint8_t position, uint8_t size) {
    return ((1u << size) - 1u) << position;
}

int sb_display_surface_valid(const sb_display_surface_t *s) {
    if (s == 0 || s->pixels == 0 || s->width == 0u || s->height == 0u ||
        (s->bits_per_pixel != 16u && s->bits_per_pixel != 24u && s->bits_per_pixel != 32u)) return 0;
    const uint64_t span = (uint64_t)s->pitch * s->height;
    if ((uint64_t)s->width * (s->bits_per_pixel / 8u) > s->pitch ||
        span == 0u || span > s->byte_length || span > UINTPTR_MAX - (uintptr_t)s->pixels) return 0;
    if (s->red_size == 0u || s->green_size == 0u || s->blue_size == 0u ||
        s->red_size > 8u || s->green_size > 8u || s->blue_size > 8u ||
        (uint16_t)s->red_position + s->red_size > s->bits_per_pixel ||
        (uint16_t)s->green_position + s->green_size > s->bits_per_pixel ||
        (uint16_t)s->blue_position + s->blue_size > s->bits_per_pixel) return 0;
    const uint32_t r = mask(s->red_position, s->red_size);
    const uint32_t g = mask(s->green_position, s->green_size);
    const uint32_t b = mask(s->blue_position, s->blue_size);
    return (r & g) == 0u && (r & b) == 0u && (g & b) == 0u;
}

static uint32_t scaled(uint8_t value, uint8_t size, uint8_t position) {
    const uint32_t maximum = (1u << size) - 1u;
    return (((uint32_t)value * maximum + 127u) / 255u) << position;
}
static uint32_t pack(const sb_display_surface_t *s, uint8_t r, uint8_t g, uint8_t b) {
    return scaled(r, s->red_size, s->red_position) | scaled(g, s->green_size, s->green_position) |
           scaled(b, s->blue_size, s->blue_position);
}
static void store(volatile uint8_t *p, uint32_t bytes, uint32_t pixel) {
    for (uint32_t i = 0u; i < bytes; ++i) p[i] = (uint8_t)(pixel >> (i * 8u));
}

int sb_display_surface_fill(sb_display_surface_t *s, uint32_t x, uint32_t y,
                             uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b) {
    if (!sb_display_surface_valid(s)) return -1;
    if (width == 0u || height == 0u) return 0;
    if (x >= s->width || y >= s->height) return -1;
    if (width > s->width - x) width = s->width - x;
    if (height > s->height - y) height = s->height - y;
    const uint32_t bytes = s->bits_per_pixel / 8u, pixel = pack(s, r, g, b);
    for (uint32_t row = 0u; row < height; ++row) {
        volatile uint8_t *p = s->pixels + (uint64_t)(y + row) * s->pitch + (uint64_t)x * bytes;
        for (uint32_t column = 0u; column < width; ++column) store(p + (uint64_t)column * bytes, bytes, pixel);
    }
    return 0;
}

int sb_display_surface_present(sb_display_surface_t *s, uint32_t x, uint32_t y,
                                uint32_t width, uint32_t height, const uint32_t *rgb, uint64_t count) {
    if (!sb_display_surface_valid(s) || rgb == 0 || width == 0u || height == 0u ||
        x >= s->width || y >= s->height || width > s->width - x || height > s->height - y ||
        (uint64_t)width * height != count || count > UINTPTR_MAX / sizeof(uint32_t) ||
        count * sizeof(uint32_t) > UINTPTR_MAX - (uintptr_t)rgb) return -1;
    const uint32_t bytes = s->bits_per_pixel / 8u;
    for (uint32_t row = 0u; row < height; ++row) {
        volatile uint8_t *p = s->pixels + (uint64_t)(y + row) * s->pitch + (uint64_t)x * bytes;
        for (uint32_t column = 0u; column < width; ++column) {
            const uint32_t pixel = rgb[(uint64_t)row * width + column];
            store(p + (uint64_t)column * bytes, bytes,
                  pack(s, (uint8_t)(pixel >> 16), (uint8_t)(pixel >> 8), (uint8_t)pixel));
        }
    }
    return 0;
}
