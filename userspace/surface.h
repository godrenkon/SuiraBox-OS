#ifndef SB_SURFACE_H
#define SB_SURFACE_H

#include <stdint.h>

#define SB_SURFACE_MAX_DAMAGE 16u

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} sb_surface_rect_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint8_t *pixels;
    sb_surface_rect_t damage[SB_SURFACE_MAX_DAMAGE];
    uint32_t damage_count;
    uint8_t full_damage;
} sb_surface_t;

#define SB_SURFACE_FORMAT_XRGB8888 1u

int sb_surface_init(sb_surface_t *surface, uint32_t width, uint32_t height,
                    uint32_t stride, uint32_t format, uint8_t *pixels);
void sb_surface_damage_all(sb_surface_t *surface);
int sb_surface_damage_rect(sb_surface_t *surface, int32_t x, int32_t y,
                           uint32_t width, uint32_t height);
int sb_surface_take_damage(sb_surface_t *surface, sb_surface_rect_t *out,
                           uint32_t capacity, uint32_t *out_count,
                           uint8_t *out_full_damage);

#endif
