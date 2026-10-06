#ifndef SB_DISPLAY_SURFACE_H
#define SB_DISPLAY_SURFACE_H
#include <stdint.h>

/* Kernel-owned little-endian RGB surface. No hardware address is exposed to
 * userspace; pitch may include padding and RGB masks may be RGB565 or RGB888. */
typedef struct {
    volatile uint8_t *pixels;
    uint64_t byte_length;
    uint32_t width, height, pitch;
    uint8_t bits_per_pixel;
    uint8_t red_position, red_size;
    uint8_t green_position, green_size;
    uint8_t blue_position, blue_size;
} sb_display_surface_t;

int sb_display_surface_valid(const sb_display_surface_t *surface);
int sb_display_surface_fill(sb_display_surface_t *surface, uint32_t x, uint32_t y,
                             uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue);
/* Strict bounds, packed rows, one 0x00RRGGBB word per pixel; high byte ignored.
 * Validate the entire request before writing any framebuffer byte. */
int sb_display_surface_present(sb_display_surface_t *surface, uint32_t x, uint32_t y,
                                uint32_t width, uint32_t height,
                                const uint32_t *rgb, uint64_t pixel_count);
#endif
