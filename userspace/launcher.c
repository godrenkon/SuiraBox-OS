#include "launcher.h"

static int valid_string(const char *s) {
    return s != 0 && s[0] != '\0';
}

static int duplicate_id(const sb_launcher_t *launcher, const char *id) {
    if (launcher == 0 || id == 0) return 0;
    for (uint32_t i = 0u; i < launcher->count; ++i) {
        const char *existing = launcher->items[i].id;
        uint32_t j = 0u;
        if (existing == 0) continue;
        while (existing[j] != '\0' && id[j] != '\0' && existing[j] == id[j]) ++j;
        if (existing[j] == '\0' && id[j] == '\0') return 1;
    }
    return 0;
}

void sb_launcher_init(sb_launcher_t *launcher) {
    if (launcher == 0) return;
    *launcher = (sb_launcher_t){0};
}

int sb_launcher_add(sb_launcher_t *launcher, const char *id, const char *label) {
    if (launcher == 0 || !valid_string(id) || !valid_string(label) ||
        launcher->count >= SB_LAUNCHER_MAX_ITEMS || duplicate_id(launcher, id)) return -1;
    launcher->items[launcher->count] = (sb_launcher_item_t){id, label, 1u};
    ++launcher->count;
    return 0;
}

int sb_launcher_set_open(sb_launcher_t *launcher, uint8_t open) {
    if (launcher == 0) return -1;
    launcher->open = open != 0u ? 1u : 0u;
    return 0;
}

int sb_launcher_move_selection(sb_launcher_t *launcher, int32_t delta) {
    int64_t next;
    if (launcher == 0 || launcher->count == 0u) return -1;
    next = (int64_t)launcher->selected + (int64_t)delta;
    while (next < 0) next += (int64_t)launcher->count;
    while (next >= (int64_t)launcher->count) next -= (int64_t)launcher->count;
    launcher->selected = (uint32_t)next;
    return 0;
}

int sb_launcher_activate(sb_launcher_t *launcher, const char **out_id) {
    if (launcher == 0 || out_id == 0 || launcher->open == 0u ||
        launcher->count == 0u || launcher->selected >= launcher->count ||
        launcher->items[launcher->selected].enabled == 0u) return -1;
    *out_id = launcher->items[launcher->selected].id;
    launcher->open = 0u;
    return 0;
}

int sb_launcher_hit_test(const sb_launcher_t *launcher, int32_t x, int32_t y,
                         uint32_t left, uint32_t top, uint32_t width,
                         uint32_t row_height, uint32_t *out_index) {
    int64_t right;
    int64_t bottom;
    int64_t rel_y;
    uint32_t index;
    if (launcher == 0 || out_index == 0 || launcher->open == 0u ||
        width == 0u || row_height == 0u || launcher->count == 0u) return -1;
    right = (int64_t)left + (int64_t)width;
    bottom = (int64_t)top + (int64_t)row_height * (int64_t)launcher->count;
    if ((int64_t)x < (int64_t)left || (int64_t)x >= right ||
        (int64_t)y < (int64_t)top || (int64_t)y >= bottom) return -1;
    rel_y = (int64_t)y - (int64_t)top;
    index = (uint32_t)(rel_y / (int64_t)row_height);
    if (index >= launcher->count) return -1;
    *out_index = index;
    return 0;
}
