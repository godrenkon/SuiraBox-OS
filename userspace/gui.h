#ifndef SB_GUI_H
#define SB_GUI_H

#include <stdint.h>

#define SB_GUI_MAX_WINDOWS 16u
#define SB_GUI_TITLEBAR_HEIGHT 36u
#define SB_GUI_CONTROL_SIZE 24u

typedef enum {
    SB_GUI_EVENT_NONE = 0,
    SB_GUI_EVENT_MOUSE_MOVE = 1,
    SB_GUI_EVENT_MOUSE_BUTTON = 2,
    SB_GUI_EVENT_KEY = 3,
    SB_GUI_EVENT_WINDOW_CLOSE = 4,
} sb_gui_event_type_t;

typedef enum {
    SB_GUI_CONTROL_NONE = 0,
    SB_GUI_CONTROL_MINIMIZE = 1,
    SB_GUI_CONTROL_MAXIMIZE = 2,
    SB_GUI_CONTROL_CLOSE = 3,
} sb_gui_control_t;

typedef struct {
    sb_gui_event_type_t type;
    int32_t x;
    int32_t y;
    int16_t dx;
    int16_t dy;
    uint8_t buttons;
    uint8_t key;
} sb_gui_event_t;

typedef struct {
    uint32_t id;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    int32_t restore_x;
    int32_t restore_y;
    uint32_t restore_width;
    uint32_t restore_height;
    uint8_t visible;
    uint8_t resizable;
    uint8_t minimized;
    uint8_t maximized;
} sb_gui_window_t;

typedef struct {
    sb_gui_window_t windows[SB_GUI_MAX_WINDOWS];
    uint32_t count;
    uint32_t next_id;
    uint32_t focused_id;
} sb_gui_window_manager_t;

void sb_gui_init(sb_gui_window_manager_t *wm);
sb_gui_window_t *sb_gui_create_window(sb_gui_window_manager_t *wm,
                                        int32_t x, int32_t y,
                                        uint32_t width, uint32_t height);
int sb_gui_destroy_window(sb_gui_window_manager_t *wm, uint32_t id);
int sb_gui_move_window(sb_gui_window_manager_t *wm, uint32_t id, int32_t x, int32_t y);
int sb_gui_resize_window(sb_gui_window_manager_t *wm, uint32_t id,
                         uint32_t width, uint32_t height);
int sb_gui_set_minimized(sb_gui_window_manager_t *wm, uint32_t id, uint8_t minimized);
int sb_gui_set_maximized(sb_gui_window_manager_t *wm, uint32_t id,
                         uint8_t maximized, uint32_t screen_width, uint32_t screen_height);
sb_gui_window_t *sb_gui_find_window(sb_gui_window_manager_t *wm, uint32_t id);
sb_gui_window_t *sb_gui_hit_test(sb_gui_window_manager_t *wm, int32_t x, int32_t y);
sb_gui_control_t sb_gui_hit_control(const sb_gui_window_t *window, int32_t x, int32_t y);
int sb_gui_focus_window(sb_gui_window_manager_t *wm, uint32_t id);

#endif
