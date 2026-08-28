#include <assert.h>
#include <stdint.h>
#include "../userspace/compositor.h"

static uint32_t display_rect_calls;
static uint32_t display_rect_pixels;
static uint32_t display_glyph_calls;
static uint32_t last_glyph_x;
static uint32_t last_glyph_y;

int sb_display_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb) {
    (void)x;
    (void)y;
    (void)rgb;
    ++display_rect_calls;
    display_rect_pixels += width * height;
    return 0;
}

int sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap, uint32_t color) {
    (void)bitmap;
    (void)color;
    ++display_glyph_calls;
    last_glyph_x = x;
    last_glyph_y = y;
    return 0;
}

int main(void) {
    sb_compositor_style_t style;
    sb_gui_window_manager_t wm;
    sb_surface_rect_t damage[2];
    uint32_t x, y, width, height;
    sb_gui_window_t *a;
    sb_gui_window_t *b;

    sb_compositor_init(&style, 1024u, 768u);
    assert(style.width == 1024u);
    assert(style.height == 768u);
    assert(style.background_rgb == 0x0C1018u);
    assert(style.cursor_rgb == 0xE9F2FFu);

    assert(sb_compositor_clip_rect(10, 20, 100u, 50u, 1024u, 768u,
                                   &x, &y, &width, &height) == 0);
    assert(x == 10u && y == 20u && width == 100u && height == 50u);

    assert(sb_compositor_clip_rect(-20, -10, 100u, 50u, 1024u, 768u,
                                   &x, &y, &width, &height) == 0);
    assert(x == 0u && y == 0u && width == 80u && height == 40u);

    assert(sb_compositor_clip_rect(1000, 750, 100u, 50u, 1024u, 768u,
                                   &x, &y, &width, &height) == 0);
    assert(x == 1000u && y == 750u && width == 24u && height == 18u);

    assert(sb_compositor_clip_rect(1024, 0, 10u, 10u, 1024u, 768u,
                                   &x, &y, &width, &height) != 0);
    assert(sb_compositor_clip_rect(0, 0, 0u, 10u, 1024u, 768u,
                                   &x, &y, &width, &height) != 0);

    sb_gui_init(&wm);
    assert(sb_compositor_visible_count(&wm) == 0u);
    a = sb_gui_create_window(&wm, 10, 10, 200u, 100u);
    b = sb_gui_create_window(&wm, 20, 20, 180u, 80u);
    assert(a != 0 && b != 0);
    assert(sb_compositor_visible_count(&wm) == 2u);
    b->minimized = 1u;
    assert(sb_compositor_visible_count(&wm) == 1u);
    a->visible = 0u;
    assert(sb_compositor_visible_count(&wm) == 0u);

    sb_gui_init(&wm);
    a = sb_gui_create_window(&wm, 100, 100, 300u, 200u);
    assert(a != 0);
    damage[0] = (sb_surface_rect_t){110, 120, 40u, 30u};
    damage[1] = (sb_surface_rect_t){900, 700, 20u, 20u};
    display_rect_calls = 0u;
    display_rect_pixels = 0u;
    sb_compositor_present_damage(&style, &wm, damage, 2u, 0u);
    assert(display_rect_calls > 0u);
    assert(display_rect_pixels > 0u);
    assert(display_rect_pixels < 1024u * 768u);

    display_rect_calls = 0u;
    sb_compositor_present_damage(&style, &wm, damage, 2u, 1u);
    assert(display_rect_calls > 0u);
    assert(display_rect_pixels > 1024u * 768u);

    display_glyph_calls = 0u;
    sb_compositor_present_cursor(&style, 100, 200);
    assert(display_glyph_calls == 1u);
    assert(last_glyph_x == 100u && last_glyph_y == 200u);
    sb_compositor_present_cursor(&style, -20, -30);
    assert(display_glyph_calls == 2u);
    assert(last_glyph_x == 0u && last_glyph_y == 0u);
    return 0;
}
