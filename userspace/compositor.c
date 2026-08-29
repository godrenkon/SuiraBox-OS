#include "compositor.h"
#if __STDC_HOSTED__ == 1
extern int sb_display_rect(uint32_t x, uint32_t y, uint32_t width,
                           uint32_t height, uint32_t rgb);
extern int sb_display_glyph(uint32_t x, uint32_t y, uint64_t bitmap,
                            uint32_t rgb);
#else
#include "syscall.h"
#endif

void sb_compositor_init(sb_compositor_style_t *style,
                        uint32_t width, uint32_t height) {
    if (style == 0) return;
    style->width = width;
    style->height = height;
    style->background_rgb = 0x0C1018u;
    style->chrome_rgb = 0x16202Cu;
    style->titlebar_rgb = 0x3A485Au;
    style->accent_rgb = 0x536F8Au;
    style->cursor_rgb = 0xE9F2FFu;
    style->close_rgb = 0xD85B68u;
    style->minimize_rgb = 0xD3A746u;
    style->maximize_rgb = 0x5EB88Cu;
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
    if (wm == 0) return 0u;
    for (uint32_t i = 0u; i < wm->count; ++i) {
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

static void render_window(const sb_compositor_style_t *style,
                          const sb_gui_window_t *window) {
    if (style == 0 || window == 0 || window->visible == 0u || window->minimized != 0u) return;
    compositor_rect(style, window->x, window->y, window->width, window->height,
                    style->titlebar_rgb);
    compositor_rect(style, window->x, window->y, window->width,
                    SB_GUI_TITLEBAR_HEIGHT, style->accent_rgb);
    if (window->width >= SB_GUI_CONTROL_SIZE * 3u) {
        compositor_rect(style, window->x + (int32_t)window->width -
                        (int32_t)(SB_GUI_CONTROL_SIZE * 3u), window->y + 6,
                        SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                        style->minimize_rgb);
        compositor_rect(style, window->x + (int32_t)window->width -
                        (int32_t)(SB_GUI_CONTROL_SIZE * 2u), window->y + 6,
                        SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                        style->maximize_rgb);
        compositor_rect(style, window->x + (int32_t)window->width -
                        (int32_t)SB_GUI_CONTROL_SIZE, window->y + 6,
                        SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                        style->close_rgb);
    }
}

static void render_taskbar(const sb_compositor_style_t *style,
                           const sb_gui_window_manager_t *wm) {
    uint32_t count;
    if (style == 0 || wm == 0 || style->height < SB_GUI_TASKBAR_HEIGHT) return;
    count = sb_gui_minimized_count(wm);
    for (uint32_t i = 0u; i < count; ++i) {
        const uint64_t left64 = (uint64_t)SB_GUI_TASKBAR_GAP +
                                (uint64_t)i * ((uint64_t)SB_GUI_TASKBAR_BUTTON_WIDTH +
                                               (uint64_t)SB_GUI_TASKBAR_GAP);
        if (left64 >= (uint64_t)style->width) break;
        uint32_t button_width = SB_GUI_TASKBAR_BUTTON_WIDTH;
        if (left64 + (uint64_t)button_width > (uint64_t)style->width)
            button_width = style->width - (uint32_t)left64;
        if (button_width == 0u) break;
        compositor_rect(style, (int32_t)left64,
                        (int32_t)style->height - (int32_t)SB_GUI_TASKBAR_HEIGHT + 12,
                        button_width, SB_GUI_TASKBAR_HEIGHT - 24u,
                        style->accent_rgb);
    }
}

static int damage_intersects_rect(const sb_surface_rect_t *damage,
                                  int32_t x, int32_t y,
                                  uint32_t width, uint32_t height) {
    const int64_t right = (int64_t)x + (int64_t)width;
    const int64_t bottom = (int64_t)y + (int64_t)height;
    const int64_t damage_right = (int64_t)damage->x + (int64_t)damage->width;
    const int64_t damage_bottom = (int64_t)damage->y + (int64_t)damage->height;
    return damage != 0 && width != 0u && height != 0u &&
           (int64_t)x < damage_right && damage->x < right &&
           (int64_t)y < damage_bottom && damage->y < bottom;
}

static void compositor_damage_rect(const sb_compositor_style_t *style,
                                   const sb_surface_rect_t *damage,
                                   int32_t x, int32_t y,
                                   uint32_t width, uint32_t height,
                                   uint32_t rgb) {
    int64_t left = x;
    int64_t top = y;
    int64_t right = left + (int64_t)width;
    int64_t bottom = top + (int64_t)height;
    int64_t damage_left = damage->x;
    int64_t damage_top = damage->y;
    int64_t damage_right = damage_left + (int64_t)damage->width;
    int64_t damage_bottom = damage_top + (int64_t)damage->height;
    int64_t clip_left = left > damage_left ? left : damage_left;
    int64_t clip_top = top > damage_top ? top : damage_top;
    int64_t clip_right = right < damage_right ? right : damage_right;
    int64_t clip_bottom = bottom < damage_bottom ? bottom : damage_bottom;

    if (style == 0 || damage == 0 || clip_left >= clip_right || clip_top >= clip_bottom) return;
    compositor_rect(style, (int32_t)clip_left, (int32_t)clip_top,
                    (uint32_t)(clip_right - clip_left),
                    (uint32_t)(clip_bottom - clip_top), rgb);
}

static void render_window_damage(const sb_compositor_style_t *style,
                                 const sb_surface_rect_t *damage,
                                 const sb_gui_window_t *window) {
    if (style == 0 || damage == 0 || window == 0 ||
        window->visible == 0u || window->minimized != 0u) return;
    if (!damage_intersects_rect(damage, window->x, window->y, window->width, window->height)) return;
    compositor_damage_rect(style, damage, window->x, window->y,
                           window->width, window->height, style->titlebar_rgb);
    compositor_damage_rect(style, damage, window->x, window->y,
                           window->width, SB_GUI_TITLEBAR_HEIGHT, style->accent_rgb);
    if (window->width >= SB_GUI_CONTROL_SIZE * 3u) {
        compositor_damage_rect(style, damage,
                               window->x + (int32_t)window->width -
                                   (int32_t)(SB_GUI_CONTROL_SIZE * 3u), window->y + 6,
                               SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                               style->minimize_rgb);
        compositor_damage_rect(style, damage,
                               window->x + (int32_t)window->width -
                                   (int32_t)(SB_GUI_CONTROL_SIZE * 2u), window->y + 6,
                               SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                               style->maximize_rgb);
        compositor_damage_rect(style, damage,
                               window->x + (int32_t)window->width -
                                   (int32_t)SB_GUI_CONTROL_SIZE, window->y + 6,
                               SB_GUI_CONTROL_SIZE, SB_GUI_CONTROL_SIZE - 8u,
                               style->close_rgb);
    }
}

static void render_taskbar_damage(const sb_compositor_style_t *style,
                                  const sb_surface_rect_t *damage,
                                  const sb_gui_window_manager_t *wm) {
    if (style == 0 || damage == 0 || wm == 0) return;
    for (uint32_t i = 0u; i < sb_gui_minimized_count(wm); ++i) {
        const uint64_t left64 = (uint64_t)SB_GUI_TASKBAR_GAP +
                                (uint64_t)i * ((uint64_t)SB_GUI_TASKBAR_BUTTON_WIDTH +
                                               (uint64_t)SB_GUI_TASKBAR_GAP);
        if (left64 >= (uint64_t)style->width) break;
        uint32_t button_width = SB_GUI_TASKBAR_BUTTON_WIDTH;
        if (left64 + (uint64_t)button_width > (uint64_t)style->width)
            button_width = style->width - (uint32_t)left64;
        if (button_width == 0u) break;
        compositor_damage_rect(style, damage, (int32_t)left64,
                               (int32_t)style->height - (int32_t)SB_GUI_TASKBAR_HEIGHT + 12,
                               button_width, SB_GUI_TASKBAR_HEIGHT - 24u,
                               style->accent_rgb);
    }
}

void sb_compositor_present_damage(const sb_compositor_style_t *style,
                                  const sb_gui_window_manager_t *wm,
                                  const sb_surface_rect_t *damage,
                                  uint32_t damage_count,
                                  uint8_t full_damage) {
    if (style == 0 || wm == 0) return;
    if (full_damage != 0u || damage == 0 || damage_count == 0u) {
        if (full_damage == 0u && damage_count == 0u) return;
        sb_compositor_present(style, wm);
        return;
    }

    for (uint32_t d = 0u; d < damage_count; ++d) {
        const sb_surface_rect_t *region = &damage[d];
        compositor_damage_rect(style, region, region->x, region->y,
                               region->width, region->height,
                               style->background_rgb);
        compositor_damage_rect(style, region, 0, 0, style->width, 72u,
                               style->chrome_rgb);
        compositor_damage_rect(style, region, 0, (int32_t)style->height - 72,
                               style->width, 72u, style->chrome_rgb);
        for (uint32_t i = 0u; i < wm->count; ++i)
            render_window_damage(style, region, &wm->windows[i]);
        compositor_damage_rect(style, region, 18, (int32_t)style->height - 60,
                               56u, 48u, style->accent_rgb);
        render_taskbar_damage(style, region, wm);
    }
}

void sb_compositor_present(const sb_compositor_style_t *style,
                           const sb_gui_window_manager_t *wm) {
    if (style == 0 || wm == 0) return;

    compositor_rect(style, 0, 0, style->width, style->height, style->background_rgb);
    compositor_rect(style, 0, 0, style->width, 72u, style->chrome_rgb);
    compositor_rect(style, 0, (int32_t)style->height - 72, style->width, 72u, style->chrome_rgb);
    for (uint32_t i = 0u; i < wm->count; ++i) render_window(style, &wm->windows[i]);
    render_taskbar(style, wm);
}

void sb_compositor_present_cursor(const sb_compositor_style_t *style,
                                  int32_t x, int32_t y) {
    static const uint64_t cursor_bitmap = 0xE0C0808080C0E0F0ULL;
    uint32_t cursor_x;
    uint32_t cursor_y;
    if (style == 0 || style->width < 8u || style->height < 8u) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int32_t)style->width - 7) x = (int32_t)style->width - 8;
    if (y >= (int32_t)style->height - 7) y = (int32_t)style->height - 8;
    cursor_x = (uint32_t)x;
    cursor_y = (uint32_t)y;
    (void)sb_display_glyph(cursor_x, cursor_y, cursor_bitmap, style->cursor_rgb);
}
