#include "surface.h"

static int rect_valid(uint32_t width, uint32_t height) {
    return width != 0u && height != 0u;
}

int sb_surface_init(sb_surface_t *surface, uint32_t width, uint32_t height,
                    uint32_t stride, uint32_t format, uint8_t *pixels) {
    if (surface == 0 || !rect_valid(width, height) ||
        width > stride / 4u || format != SB_SURFACE_FORMAT_XRGB8888) return -1;
    surface->width = width;
    surface->height = height;
    surface->stride = stride;
    surface->format = format;
    surface->pixels = pixels;
    surface->damage_count = 0u;
    surface->full_damage = 1u;
    return 0;
}

void sb_surface_damage_all(sb_surface_t *surface) {
    if (surface == 0) return;
    surface->damage_count = 0u;
    surface->full_damage = 1u;
}

int sb_surface_damage_rect(sb_surface_t *surface, int32_t x, int32_t y,
                           uint32_t width, uint32_t height) {
    int64_t right;
    int64_t bottom;
    int64_t clipped_x;
    int64_t clipped_y;
    int64_t clipped_right;
    int64_t clipped_bottom;
    if (surface == 0 || !rect_valid(width, height)) return -1;
    if (surface->full_damage != 0u) return 0;

    right = (int64_t)x + (int64_t)width;
    bottom = (int64_t)y + (int64_t)height;
    clipped_x = x < 0 ? 0 : x;
    clipped_y = y < 0 ? 0 : y;
    clipped_right = right > (int64_t)surface->width ? (int64_t)surface->width : right;
    clipped_bottom = bottom > (int64_t)surface->height ? (int64_t)surface->height : bottom;
    if (clipped_x >= clipped_right || clipped_y >= clipped_bottom) return 0;

    if (surface->damage_count >= SB_SURFACE_MAX_DAMAGE) {
        surface->damage_count = 0u;
        surface->full_damage = 1u;
        return 0;
    }
    surface->damage[surface->damage_count++] = (sb_surface_rect_t){
        clipped_x, clipped_y,
        (uint32_t)(clipped_right - clipped_x),
        (uint32_t)(clipped_bottom - clipped_y)
    };
    return 0;
}

int sb_surface_take_damage(sb_surface_t *surface, sb_surface_rect_t *out,
                           uint32_t capacity, uint32_t *out_count,
                           uint8_t *out_full_damage) {
    if (surface == 0 || out_count == 0 || out_full_damage == 0) return -1;
    if (surface->full_damage != 0u) {
        *out_count = 0u;
        *out_full_damage = 1u;
        surface->full_damage = 0u;
        surface->damage_count = 0u;
        return 0;
    }
    if (surface->damage_count > capacity ||
        (surface->damage_count != 0u && out == 0)) return -1;
    for (uint32_t i = 0u; i < surface->damage_count; ++i) out[i] = surface->damage[i];
    *out_count = surface->damage_count;
    *out_full_damage = 0u;
    surface->damage_count = 0u;
    return 0;
}
