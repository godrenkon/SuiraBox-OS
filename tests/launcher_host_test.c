#include <assert.h>
#include <stdint.h>
#include "../userspace/launcher.h"

int main(void) {
    sb_launcher_t launcher;
    const char *id = 0;
    uint32_t index = 99u;

    sb_launcher_init(&launcher);
    assert(launcher.count == 0u && launcher.open == 0u && launcher.selected == 0u);
    assert(sb_launcher_add(&launcher, "settings", "Settings") == 0);
    assert(sb_launcher_add(&launcher, "files", "File Manager") == 0);
    assert(sb_launcher_add(&launcher, "terminal", "Terminal") == 0);
    assert(sb_launcher_add(&launcher, "settings", "Settings Again") != 0);
    assert(launcher.count == 3u);

    assert(sb_launcher_set_open(&launcher, 1u) == 0);
    assert(sb_launcher_move_selection(&launcher, -1) == 0);
    assert(launcher.selected == 2u);
    assert(sb_launcher_move_selection(&launcher, 1) == 0);
    assert(launcher.selected == 0u);

    assert(sb_launcher_hit_test(&launcher, 20, 25, 10u, 10u, 200u, 40u, &index) == 0);
    assert(index == 0u);
    assert(sb_launcher_hit_test(&launcher, 20, 65, 10u, 10u, 200u, 40u, &index) == 0);
    assert(index == 1u);
    assert(sb_launcher_hit_test(&launcher, 20, 130, 10u, 10u, 200u, 40u, &index) != 0);
    assert(sb_launcher_hit_test(&launcher, 250, 25, 10u, 10u, 200u, 40u, &index) != 0);

    launcher.items[1].enabled = 0u;
    launcher.selected = 0u;
    assert(sb_launcher_move_selection(&launcher, 1) == 0);
    assert(launcher.selected == 2u);
    assert(sb_launcher_move_selection(&launcher, 1) == 0);
    assert(launcher.selected == 0u);
    assert(sb_launcher_move_selection(&launcher, -1) == 0);
    assert(launcher.selected == 2u);

    assert(sb_launcher_activate(&launcher, &id) == 0);
    assert(id != 0 && id[0] == 't');
    assert(launcher.open == 0u);
    assert(sb_launcher_activate(&launcher, &id) != 0);

    launcher.selected = 1u;
    assert(sb_launcher_set_open(&launcher, 1u) == 0);
    assert(sb_launcher_activate(&launcher, &id) != 0);
    assert(launcher.open == 1u);
    return 0;
}
