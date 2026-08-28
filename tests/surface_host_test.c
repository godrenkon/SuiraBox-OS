#include <assert.h>
#include <stdint.h>
#include "../userspace/surface.h"

int main(void) {
    sb_surface_t surface;
    sb_surface_rect_t damage[2];
    uint32_t count;
    uint8_t full;

    assert(sb_surface_init(&surface, 640u, 480u, 2560u,
                           SB_SURFACE_FORMAT_XRGB8888, 0) == 0);
    assert(sb_surface_init(&surface, UINT32_MAX, 1u, UINT32_MAX,
                           SB_SURFACE_FORMAT_XRGB8888, 0) != 0);
    assert(sb_surface_init(&surface, 640u, 480u, 2559u,
                           SB_SURFACE_FORMAT_XRGB8888, 0) != 0);

    assert(surface.full_damage == 1u);
    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 1u && count == 0u);

    assert(sb_surface_damage_rect(&surface, -10, -20, 100u, 70u) == 0);
    assert(sb_surface_damage_rect(&surface, 620, 460, 40u, 40u) == 0);
    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 0u && count == 2u);
    assert(damage[0].x == 0 && damage[0].y == 0);
    assert(damage[0].width == 90u && damage[0].height == 50u);
    assert(damage[1].x == 620 && damage[1].y == 460);
    assert(damage[1].width == 20u && damage[1].height == 20u);

    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 0u && count == 0u);
    return 0;
}
