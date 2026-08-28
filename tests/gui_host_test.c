#include <assert.h>
#include <stdint.h>
#include "../userspace/gui.h"

int main(void) {
    sb_gui_window_manager_t wm;
    sb_gui_window_t *back;
    sb_gui_window_t *front;
    sb_gui_window_t *top;

    sb_gui_init(&wm);
    assert(wm.count == 0u);
    assert(wm.focused_id == 0u);

    back = sb_gui_create_window(&wm, 10, 20, 200u, 120u);
    front = sb_gui_create_window(&wm, 40, 50, 160u, 100u);
    assert(back != 0 && front != 0);
    assert(wm.count == 2u);
    assert(wm.focused_id == front->id);

    assert(sb_gui_hit_test(&wm, 45, 55) == front);
    assert(sb_gui_hit_test(&wm, 15, 25) == back);
    assert(sb_gui_hit_test(&wm, 1000, 1000) == 0);

    top = sb_gui_create_window(&wm, 30, 40, 180u, 110u);
    assert(top != 0);
    assert(sb_gui_hit_test(&wm, 50, 60) == top);
    assert(sb_gui_focus_window(&wm, back->id) == 0);
    assert(wm.focused_id == back->id);
    assert(sb_gui_hit_test(&wm, 50, 60) == back);

    assert(sb_gui_move_window(&wm, front->id, 100, 100) == 0);
    assert(sb_gui_hit_test(&wm, 105, 105) == front);
    assert(sb_gui_resize_window(&wm, front->id, 320u, 240u) == 0);
    assert(front->width == 320u && front->height == 240u);

    front->visible = 0u;
    assert(sb_gui_hit_test(&wm, 105, 105) == back);
    front->visible = 1u;
    assert(sb_gui_focus_window(&wm, front->id) == 0);
    assert(wm.focused_id == front->id);

    assert(sb_gui_destroy_window(&wm, front->id) == 0);
    assert(wm.count == 2u);
    assert(sb_gui_find_window(&wm, front->id) == 0);
    assert(sb_gui_destroy_window(&wm, 0u) != 0);
    return 0;
}
