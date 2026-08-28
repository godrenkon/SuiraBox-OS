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
    assert(surface.full_damage == 1u);
    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 1u && count == 0u);

    assert(sb_surface_damage_rect(&surface, 10, 20, 100u, 50u) == 0);
    assert(sb_surface_damage_rect(&surface, 200, 220, 50u, 40u) == 0);
    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 0u && count == 2u);
    assert(damage[0].x == 10 && damage[0].y == 20);
    assert(damage[1].x == 200 && damage[1].y == 220);

    assert(sb_surface_take_damage(&surface, damage, 2u, &count, &full) == 0);
    assert(full == 0u && count == 0u);

    for (uint32_t i = 0u; i < SB_SURFACE_MAX_DAMAGE; ++i) {
        assert(sb_surface_damage_rect(&surface, (int32_t)i, 0, 1u, 1u) == 0);
    }
    assert(sb_surface_damage_rect(&surface, 100, 100, 1u, 1u) == 0);
    assert(surface.full_damage == 1u);
    return 0;
}
