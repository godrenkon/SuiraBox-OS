#ifndef SB_COMPOSITOR_H
#define SB_COMPOSITOR_H

#include <stdint.h>
#include "gui.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t background_rgb;
    uint32_t chrome_rgb;
    uint32_t titlebar_rgb;
    uint32_t accent_rgb;
} sb_compositor_style_t;

void sb_compositor_init(sb_compositor_style_t *style,
                        uint32_t width, uint32_t height);
int sb_compositor_clip_rect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                            uint32_t screen_width, uint32_t screen_height,
                            uint32_t *out_x, uint32_t *out_y,
                            uint32_t *out_width, uint32_t *out_height);
uint32_t sb_compositor_visible_count(const sb_gui_window_manager_t *wm);
void sb_compositor_present(const sb_compositor_style_t *style,
                           const sb_gui_window_manager_t *wm);

#endif
