#include <assert.h>
#include <stdint.h>
#include "../userspace/settings_view.h"

int main(void) {
    sb_settings_view_t view;
    sb_settings_view_init(&view, 1u, 0u);

    assert(view.open == 1u);
    assert(view.selected == 0u);
    assert(sb_settings_view_mask(&view) == 0u);

    assert(sb_settings_view_toggle(&view) == 0);
    assert(sb_settings_view_mask(&view) == 1u);
    assert(sb_settings_view_toggle(&view) == 0);
    assert(sb_settings_view_mask(&view) == 0u);

    assert(sb_settings_view_move(&view, -1) == 0);
    assert(view.selected == SB_SETTINGS_ITEM_COUNT - 1u);
    assert(sb_settings_view_move(&view, 5) == 0);
    assert(view.selected == 0u);
    assert(sb_settings_view_move(&view, INT32_MAX) == 0);
    assert(view.selected < SB_SETTINGS_ITEM_COUNT);
    assert(sb_settings_view_move(&view, INT32_MIN) == 0);
    assert(view.selected < SB_SETTINGS_ITEM_COUNT);

    assert(sb_settings_view_set_remote_available(&view, 1u) == 0);
    assert(view.remote_available == 1u);
    assert(sb_settings_view_set_remote_available(&view, 0u) == 0);
    assert(view.remote_available == 0u);
    return 0;
}
