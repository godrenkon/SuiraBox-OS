#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include "../userspace/gui.h"

int main(void) {
    sb_gui_window_manager_t wm;
    sb_gui_window_t *back;
    sb_gui_window_t *front;
    sb_gui_window_t *top;
    sb_gui_window_t extreme;
    uint32_t back_id;
    uint32_t front_id;
    const uint32_t screen_width = 1024u;
    const uint32_t screen_height = 768u;

    sb_gui_init(&wm);
    assert(wm.count == 0u);
    assert(wm.focused_id == 0u);

    back = sb_gui_create_window(&wm, 10, 20, 200u, 120u);
    front = sb_gui_create_window(&wm, 40, 50, 160u, 100u);
    assert(back != 0 && front != 0);
    back_id = back->id;
    front_id = front->id;
    assert(wm.count == 2u);
    assert(wm.focused_id == front_id);

    assert(sb_gui_hit_test(&wm, 45, 55)->id == front_id);
    assert(sb_gui_hit_test(&wm, 15, 25)->id == back_id);
    assert(sb_gui_hit_test(&wm, 1000, 1000) == 0);

    assert(sb_gui_hit_control(front, 64, 60) == SB_GUI_CONTROL_MINIMIZE);
    assert(sb_gui_hit_control(front, 88, 60) == SB_GUI_CONTROL_MAXIMIZE);
    assert(sb_gui_hit_control(front, 112, 60) == SB_GUI_CONTROL_CLOSE);
    assert(sb_gui_hit_control(front, 100, 60) == SB_GUI_CONTROL_NONE);
    assert(sb_gui_hit_control(front, 191, 90) == SB_GUI_CONTROL_NONE);
    assert(sb_gui_hit_control(0, 191, 60) == SB_GUI_CONTROL_NONE);

    top = sb_gui_create_window(&wm, 30, 40, 180u, 110u);
    assert(top != 0);
    assert(sb_gui_hit_test(&wm, 50, 60)->id == top->id);
    assert(sb_gui_focus_window(&wm, back_id) == 0);
    assert(wm.focused_id == back_id);
    assert(sb_gui_hit_test(&wm, 50, 60)->id == back_id);

    front = sb_gui_find_window(&wm, front_id);
    assert(front != 0);
    assert(sb_gui_move_window(&wm, front_id, 100, 100) == 0);
    assert(sb_gui_hit_test(&wm, 105, 105)->id == back_id);
    assert(sb_gui_focus_window(&wm, front_id) == 0);
    assert(sb_gui_hit_test(&wm, 105, 105)->id == front_id);
    assert(sb_gui_resize_window(&wm, front_id, 320u, 240u) == 0);
    front = sb_gui_find_window(&wm, front_id);
    assert(front != 0);
    assert(front->width == 320u && front->height == 240u);
    assert(sb_gui_hit_control(front, 348, 106) == SB_GUI_CONTROL_MINIMIZE);
    assert(sb_gui_hit_control(front, 372, 106) == SB_GUI_CONTROL_MAXIMIZE);
    assert(sb_gui_hit_control(front, 396, 106) == SB_GUI_CONTROL_CLOSE);

    extreme = (sb_gui_window_t){
        99u, INT32_MAX - 8, INT32_MAX - 8, UINT32_MAX, UINT32_MAX,
        INT32_MAX - 8, INT32_MAX - 8, UINT32_MAX, UINT32_MAX,
        1u, 1u, 0u, 0u
    };
    assert(sb_gui_hit_control(&extreme, INT32_MAX, INT32_MAX) == SB_GUI_CONTROL_NONE);
    assert(sb_gui_hit_control(&extreme, INT32_MIN, INT32_MIN) == SB_GUI_CONTROL_NONE);

    assert(sb_gui_set_maximized(&wm, front_id, 1u, screen_width, screen_height) == 0);
    front = sb_gui_find_window(&wm, front_id);
    assert(front != 0);
    assert(front->maximized == 1u);
    assert(front->x == 0 && front->y == SB_GUI_TITLEBAR_HEIGHT);
    assert(front->width == screen_width);
    assert(front->height == screen_height - SB_GUI_TITLEBAR_HEIGHT);
    assert(sb_gui_move_window(&wm, front_id, 10, 10) != 0);
    assert(sb_gui_resize_window(&wm, front_id, 200u, 100u) != 0);
    assert(sb_gui_hit_control(front, 100, 80) == SB_GUI_CONTROL_NONE);
    assert(sb_gui_set_maximized(&wm, front_id, 0u, screen_width, screen_height) == 0);
    front = sb_gui_find_window(&wm, front_id);
    assert(front != 0);
    assert(front->maximized == 0u);
    assert(front->x == 100 && front->y == 100);
    assert(front->width == 320u && front->height == 240u);

    assert(sb_gui_set_minimized(&wm, front_id, 1u) == 0);
    assert(wm.focused_id != front_id);
    assert(sb_gui_hit_test(&wm, 105, 105)->id != front_id);
    assert(sb_gui_hit_control(front, 396, 106) == SB_GUI_CONTROL_NONE);
    assert(sb_gui_set_minimized(&wm, front_id, 0u) == 0);
    assert(wm.focused_id == front_id);
    assert(sb_gui_hit_test(&wm, 105, 105)->id == front_id);

    front = sb_gui_find_window(&wm, front_id);
    assert(front != 0);
    front->visible = 0u;
    assert(sb_gui_hit_test(&wm, 105, 105)->id != front_id);
    assert(sb_gui_hit_control(front, 396, 106) == SB_GUI_CONTROL_NONE);
    front->visible = 1u;
    assert(sb_gui_focus_window(&wm, front_id) == 0);
    assert(wm.focused_id == front_id);

    assert(sb_gui_set_minimized(&wm, top->id, 1u) == 0);
    assert(top->minimized == 1u);
    assert(sb_gui_destroy_window(&wm, front_id) == 0);
    assert(wm.count == 2u);
    assert(wm.focused_id == back_id);
    assert(sb_gui_find_window(&wm, front_id) == 0);
    assert(sb_gui_destroy_window(&wm, 0u) != 0);
    return 0;
}
