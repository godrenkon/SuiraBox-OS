#include "gui.h"

static sb_gui_window_t *find_slot(sb_gui_window_manager_t *wm, uint32_t id) {
    uint32_t i;
    if (wm == 0 || id == 0u) return 0;
    for (i = 0u; i < wm->count; ++i) {
        if (wm->windows[i].id == id) return &wm->windows[i];
    }
    return 0;
}

static uint32_t choose_fallback_focus(const sb_gui_window_manager_t *wm) {
    if (wm == 0) return 0u;
    for (uint32_t i = wm->count; i != 0u; --i) {
        const sb_gui_window_t *candidate = &wm->windows[i - 1u];
        if (candidate->visible != 0u && candidate->minimized == 0u) return candidate->id;
    }
    return 0u;
}

void sb_gui_init(sb_gui_window_manager_t *wm) {
    uint32_t i;
    if (wm == 0) return;
    wm->count = 0u;
    wm->next_id = 1u;
    wm->focused_id = 0u;
    for (i = 0u; i < SB_GUI_MAX_WINDOWS; ++i) wm->windows[i] = (sb_gui_window_t){0};
}

sb_gui_window_t *sb_gui_create_window(sb_gui_window_manager_t *wm,
                                        int32_t x, int32_t y,
                                        uint32_t width, uint32_t height) {
    sb_gui_window_t *window;
    if (wm == 0 || wm->count >= SB_GUI_MAX_WINDOWS || width < SB_GUI_MIN_WINDOW_WIDTH ||
        height < SB_GUI_MIN_WINDOW_HEIGHT) return 0;
    window = &wm->windows[wm->count++];
    window->id = wm->next_id++;
    if (window->id == 0u) window->id = wm->next_id++;
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->restore_x = x;
    window->restore_y = y;
    window->restore_width = width;
    window->restore_height = height;
    window->visible = 1u;
    window->resizable = 1u;
    window->minimized = 0u;
    window->maximized = 0u;
    wm->focused_id = window->id;
    return window;
}

int sb_gui_destroy_window(sb_gui_window_manager_t *wm, uint32_t id) {
    uint32_t i;
    if (wm == 0 || id == 0u) return -1;
    for (i = 0u; i < wm->count; ++i) {
        if (wm->windows[i].id != id) continue;
        if (i + 1u < wm->count) wm->windows[i] = wm->windows[wm->count - 1u];
        --wm->count;
        if (wm->focused_id == id) wm->focused_id = choose_fallback_focus(wm);
        return 0;
    }
    return -1;
}

int sb_gui_move_window(sb_gui_window_manager_t *wm, uint32_t id, int32_t x, int32_t y) {
    sb_gui_window_t *window = find_slot(wm, id);
    if (window == 0 || window->maximized != 0u) return -1;
    window->x = x;
    window->y = y;
    window->restore_x = x;
    window->restore_y = y;
    return 0;
}

int sb_gui_resize_window(sb_gui_window_manager_t *wm, uint32_t id,
                         uint32_t width, uint32_t height) {
    sb_gui_window_t *window = find_slot(wm, id);
    if (window == 0 || width < SB_GUI_MIN_WINDOW_WIDTH ||
        height < SB_GUI_MIN_WINDOW_HEIGHT || window->resizable == 0u ||
        window->maximized != 0u) return -1;
    window->width = width;
    window->height = height;
    window->restore_width = width;
    window->restore_height = height;
    return 0;
}

int sb_gui_set_minimized(sb_gui_window_manager_t *wm, uint32_t id, uint8_t minimized) {
    sb_gui_window_t *window = find_slot(wm, id);
    if (window == 0) return -1;
    window->minimized = minimized != 0u ? 1u : 0u;
    if (window->minimized != 0u && wm->focused_id == id) {
        wm->focused_id = choose_fallback_focus(wm);
    } else if (window->minimized == 0u) {
        wm->focused_id = id;
    }
    return 0;
}

int sb_gui_set_maximized(sb_gui_window_manager_t *wm, uint32_t id,
                         uint8_t maximized, uint32_t screen_width, uint32_t screen_height) {
    sb_gui_window_t *window = find_slot(wm, id);
    if (window == 0 || window->resizable == 0u || window->minimized != 0u ||
        screen_width == 0u || screen_height <= SB_GUI_TITLEBAR_HEIGHT) return -1;

    if (maximized != 0u) {
        if (window->maximized == 0u) {
            window->restore_x = window->x;
            window->restore_y = window->y;
            window->restore_width = window->width;
            window->restore_height = window->height;
        }
        window->x = 0;
        window->y = SB_GUI_TITLEBAR_HEIGHT;
        window->width = screen_width;
        window->height = screen_height - SB_GUI_TITLEBAR_HEIGHT;
        window->maximized = 1u;
    } else {
        window->x = window->restore_x;
        window->y = window->restore_y;
        window->width = window->restore_width;
        window->height = window->restore_height;
        window->maximized = 0u;
    }
    wm->focused_id = id;
    return 0;
}

int sb_gui_restore_window(sb_gui_window_manager_t *wm, uint32_t id) {
    sb_gui_window_t *window = find_slot(wm, id);
    if (window == 0 || window->visible == 0u) return -1;
    window->minimized = 0u;
    return sb_gui_focus_window(wm, id);
}

sb_gui_window_t *sb_gui_find_window(sb_gui_window_manager_t *wm, uint32_t id) {
    return find_slot(wm, id);
}

sb_gui_window_t *sb_gui_hit_test(sb_gui_window_manager_t *wm, int32_t x, int32_t y) {
    if (wm == 0) return 0;
    for (uint32_t i = wm->count; i != 0u; --i) {
        sb_gui_window_t *window = &wm->windows[i - 1u];
        if (window->visible == 0u || window->minimized != 0u) continue;
        if (x < window->x || y < window->y) continue;
        if ((uint32_t)(x - window->x) >= window->width) continue;
        if ((uint32_t)(y - window->y) >= window->height) continue;
        return window;
    }
    return 0;
}

sb_gui_control_t sb_gui_hit_control(const sb_gui_window_t *window, int32_t x, int32_t y) {
    const int64_t window_left = window != 0 ? (int64_t)window->x : 0;
    const int64_t window_top = window != 0 ? (int64_t)window->y : 0;
    const int64_t window_right = window_left + (window != 0 ? (int64_t)window->width : 0);
    const int64_t titlebar_bottom = window_top + (int64_t)SB_GUI_TITLEBAR_HEIGHT;
    const int64_t min_left = window_right - (int64_t)(SB_GUI_CONTROL_SIZE * 3u);
    const int64_t max_left = window_right - (int64_t)(SB_GUI_CONTROL_SIZE * 2u);
    const int64_t close_left = window_right - (int64_t)SB_GUI_CONTROL_SIZE;

    if (window == 0 || window->visible == 0u || window->minimized != 0u) return SB_GUI_CONTROL_NONE;
    if (window->width < SB_GUI_CONTROL_SIZE * 3u) return SB_GUI_CONTROL_NONE;
    if ((int64_t)y < window_top || (int64_t)y >= titlebar_bottom) return SB_GUI_CONTROL_NONE;
    if ((int64_t)x >= close_left && (int64_t)x < window_right) return SB_GUI_CONTROL_CLOSE;
    if ((int64_t)x >= max_left && (int64_t)x < close_left) return SB_GUI_CONTROL_MAXIMIZE;
    if ((int64_t)x >= min_left && (int64_t)x < max_left) return SB_GUI_CONTROL_MINIMIZE;
    return SB_GUI_CONTROL_NONE;
}

sb_gui_resize_edge_t sb_gui_hit_resize(const sb_gui_window_t *window, int32_t x, int32_t y) {
    const int64_t left = window != 0 ? (int64_t)window->x : 0;
    const int64_t top = window != 0 ? (int64_t)window->y : 0;
    const int64_t right = left + (window != 0 ? (int64_t)window->width : 0);
    const int64_t bottom = top + (window != 0 ? (int64_t)window->height : 0);
    const int64_t grip = (int64_t)SB_GUI_RESIZE_GRIP_SIZE;

    if (window == 0 || window->visible == 0u || window->minimized != 0u ||
        window->maximized != 0u || window->resizable == 0u) return SB_GUI_RESIZE_NONE;
    if ((int64_t)x >= right - grip && (int64_t)x < right &&
        (int64_t)y >= bottom - grip && (int64_t)y < bottom)
        return SB_GUI_RESIZE_BOTTOM_RIGHT;
    if ((int64_t)x >= right - grip && (int64_t)x < right &&
        (int64_t)y >= top + (int64_t)SB_GUI_TITLEBAR_HEIGHT && (int64_t)y < bottom)
        return SB_GUI_RESIZE_RIGHT;
    if ((int64_t)y >= bottom - grip && (int64_t)y < bottom &&
        (int64_t)x >= left && (int64_t)x < right)
        return SB_GUI_RESIZE_BOTTOM;
    return SB_GUI_RESIZE_NONE;
}

uint32_t sb_gui_minimized_count(const sb_gui_window_manager_t *wm) {
    uint32_t count = 0u;
    if (wm == 0) return 0u;
    for (uint32_t i = 0u; i < wm->count; ++i) {
        if (wm->windows[i].visible != 0u && wm->windows[i].minimized != 0u) ++count;
    }
    return count;
}

sb_gui_window_t *sb_gui_minimized_at(sb_gui_window_manager_t *wm, uint32_t index) {
    if (wm == 0) return 0;
    for (uint32_t i = 0u; i < wm->count; ++i) {
        sb_gui_window_t *window = &wm->windows[i];
        if (window->visible != 0u && window->minimized != 0u) {
            if (index == 0u) return window;
            --index;
        }
    }
    return 0;
}

uint32_t sb_gui_hit_taskbar(const sb_gui_window_manager_t *wm, int32_t x, int32_t y,
                            uint32_t screen_width, uint32_t screen_height) {
    uint32_t index;
    int64_t top;
    if (wm == 0 || screen_width == 0u || screen_height < SB_GUI_TASKBAR_HEIGHT) return 0u;
    top = (int64_t)screen_height - (int64_t)SB_GUI_TASKBAR_HEIGHT;
    if ((int64_t)y < top || (int64_t)y >= (int64_t)screen_height || x < 0) return 0u;
    for (index = 0u; index < sb_gui_minimized_count(wm); ++index) {
        const int64_t left = (int64_t)SB_GUI_TASKBAR_GAP +
                             (int64_t)index * ((int64_t)SB_GUI_TASKBAR_BUTTON_WIDTH +
                                              (int64_t)SB_GUI_TASKBAR_GAP);
        const int64_t right = left + (int64_t)SB_GUI_TASKBAR_BUTTON_WIDTH;
        if ((int64_t)x >= left && (int64_t)x < right) {
            sb_gui_window_t *window = sb_gui_minimized_at((sb_gui_window_manager_t *)wm, index);
            return window != 0 ? window->id : 0u;
        }
    }
    return 0u;
}

int sb_gui_focus_window(sb_gui_window_manager_t *wm, uint32_t id) {
    uint32_t i;
    sb_gui_window_t focused;
    if (wm == 0 || id == 0u) return -1;
    for (i = 0u; i < wm->count; ++i) {
        if (wm->windows[i].id != id) continue;
        if (wm->windows[i].visible == 0u || wm->windows[i].minimized != 0u) return -1;
        if (i + 1u == wm->count) {
            wm->focused_id = id;
            return 0;
        }
        focused = wm->windows[i];
        for (; i + 1u < wm->count; ++i) wm->windows[i] = wm->windows[i + 1u];
        wm->windows[wm->count - 1u] = focused;
        wm->focused_id = id;
        return 0;
    }
    return -1;
}
