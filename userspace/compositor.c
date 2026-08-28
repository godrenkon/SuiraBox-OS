#include "compositor.h"
#include "syscall.h"

void sb_compositor_init(sb_compositor_style_t *style,
                        uint32_t width, uint32_t height) {
    if (style == 0) return;
    style->width = width;
    style->height = height;
    style->background_rgb = 0x0C1018u;
    style->chrome_rgb = 0x16202Cu;
    style->titlebar_rgb = 0x3A485Au;
    style->accent_rgb = 0x536F8Au;
}

int sb_compositor_clip_rect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                            uint32_t screen_width, uint32_t screen_height,
                            uint32_t *out_x, uint32_t *out_y,
                            uint32_t *out_width, uint32_t *out_height) {
    int64_t right;
    int64_t bottom;
    int64_t clipped_x;
    int64_t clipped_y;
    int64_t clipped_right;
    int64_t clipped_bottom;

    if (out_x == 0 || out_y == 0 || out_width == 0 || out_height == 0) return -1;
    if (width == 0u || height == 0u || screen_width == 0u || screen_height == 0u) return -1;

    right = (int64_t)x + (int64_t)width;
    bottom = (int64_t)y + (int64_t)height;
    clipped_x = x < 0 ? 0 : x;
    clipped_y = y < 0 ? 0 : y;
    clipped_right = right > (int64_t)screen_width ? (int64_t)screen_width : right;
    clipped_bottom = bottom > (int64_t)screen_height ? (int64_t)screen_height : bottom;

    if (clipped_x >= clipped_right || clipped_y >= clipped_bottom) return -1;

    *out_x = (uint32_t)clipped_x;
    *out_y = (uint32_t)clipped_y;
    *out_width = (uint32_t)(clipped_right - clipped_x);
    *out_height = (uint32_t)(clipped_bottom - clipped_y);
    return 0;
}

uint32_t sb_compositor_visible_count(const sb_gui_window_manager_t *wm) {
    uint32_t count = 0u;
    uint32_t i;
    if (wm == 0) return 0u;
    for (i = 0u; i < wm->count; ++i) {
        const sb_gui_window_t *window = &wm->windows[i];
        if (window->visible != 0u && window->minimized == 0u) ++count;
    }
    return count;
}

static void compositor_rect(const sb_compositor_style_t *style,
                            int32_t x, int32_t y, uint32_t width, uint32_t height,
                            uint32_t rgb) {
    uint32_t out_x;
    uint32_t out_y;
    uint32_t out_width;
    uint32_t out_height;
    if (style == 0) return;
    if (sb_compositor_clip_rect(x, y, width, height,
                                style->width, style->height,
                                &out_x, &out_y, &out_width, &out_height) != 0) return;
    (void)sb_display_rect(out_x, out_y, out_width, out_height, rgb);
}

void sb_compositor_present(const sb_compositor_style_t *style,
                           const sb_gui_window_manager_t *wm) {
    uint32_t i;
    if (style == 0 || wm == 0) return;

    compositor_rect(style, 0, 0, style->width, style->height, style->background_rgb);
    compositor_rect(style, 0, 0, style->width, 72u, style->chrome_rgb);
    compositor_rect(style, 0, (int32_t)style->height - 72, style->width, 72u, style->chrome_rgb);

    /* Windows are submitted in manager order: later entries are visually above earlier ones. */
    for (i = 0u; i < wm->count; ++i) {
        const sb_gui_window_t *window = &wm->windows[i];
        if (window->visible == 0u || window->minimized != 0u) continue;
        compositor_rect(style, window->x, window->y,
                        window->width, window->height, style->titlebar_rgb);
        compositor_rect(style, window->x, window->y,
                        window->width, 36u, style->accent_rgb);
    }

    compositor_rect(style, 18, (int32_t)style->height - 60, 56u, 48u, style->accent_rgb);
}
