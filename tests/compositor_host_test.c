#include <assert.h>
#include <stdint.h>
#include "../userspace/compositor.h"

int main(void) {
    sb_compositor_style_t style;
    sb_gui_window_manager_t wm;
    uint32_t x, y, width, height;
    sb_gui_window_t *a;
    sb_gui_window_t *b;

    sb_compositor_init(&style, 1024u, 768u);
    assert(style.width == 1024u);
    assert(style.height == 768u);
    assert(style.background_rgb == 0x0C1018u);

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
    return 0;
}
