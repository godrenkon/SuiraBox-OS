#include "desktop_bootstrap.h"
#include "framebuffer.h"
#include <stdint.h>

static int draw_panel(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (sb_framebuffer_fill_rect(x, y, width, height, 28u, 34u, 46u) != 0) return -1;
    if (sb_framebuffer_fill_rect(x + 1u, y + 1u, width - 2u, 1u, 68u, 78u, 96u) != 0) return -1;
    return 0;
}

int sb_desktop_bootstrap_render(void) {
    const sb_framebuffer_info_t *fb = sb_framebuffer_info();
    uint32_t bar_height;
    uint32_t dock_height;
    uint32_t panel_x;
    uint32_t panel_y;
    uint32_t panel_width;
    uint32_t panel_height;

    if (!sb_framebuffer_available() || fb == 0u) return -1;
    if (fb->width < 640u || fb->height < 480u) return -1;

    bar_height = fb->height >= 720u ? 48u : 40u;
    dock_height = fb->height >= 720u ? 72u : 60u;

    if (sb_framebuffer_fill_rect(0u, 0u, fb->width, bar_height, 18u, 22u, 30u) != 0) return -1;
    if (sb_framebuffer_fill_rect(0u, bar_height, fb->width, 2u, 54u, 66u, 82u) != 0) return -1;

    panel_width = fb->width >= 1100u ? 760u : fb->width - 160u;
    panel_height = fb->height >= 760u ? 440u : fb->height - bar_height - dock_height - 80u;
    panel_x = (fb->width - panel_width) / 2u;
    panel_y = bar_height + 48u;

    if (draw_panel(panel_x, panel_y, panel_width, panel_height) != 0) return -1;
    if (sb_framebuffer_fill_rect(panel_x + 16u, panel_y + 16u,
                                 panel_width - 32u, 34u,
                                 38u, 46u, 60u) != 0) return -1;
    if (sb_framebuffer_fill_rect(panel_x + 32u, panel_y + 76u,
                                 (panel_width - 80u) / 2u, 110u,
                                 42u, 54u, 72u) != 0) return -1;
    if (sb_framebuffer_fill_rect(panel_x + 48u + (panel_width - 80u) / 2u,
                                 panel_y + 76u,
                                 (panel_width - 80u) / 2u, 110u,
                                 42u, 54u, 72u) != 0) return -1;

    if (sb_framebuffer_fill_rect(0u, fb->height - dock_height,
                                 fb->width, dock_height,
                                 20u, 25u, 34u) != 0) return -1;
    if (sb_framebuffer_fill_rect((fb->width - 320u) / 2u,
                                 fb->height - dock_height + 14u,
                                 320u, dock_height - 28u,
                                 36u, 43u, 56u) != 0) return -1;

    return 0;
}
