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

static int first_enabled(const sb_launcher_t *launcher, uint32_t *index) {
    if (launcher == 0 || index == 0) return -1;
    for (uint32_t i = 0u; i < launcher->count; ++i) {
        if (launcher->items[i].enabled != 0u) {
            *index = i;
            return 0;
        }
    }
    return -1;
}

static int enabled_count(const sb_launcher_t *launcher, uint32_t *count) {
    uint32_t total = 0u;
    if (launcher == 0 || count == 0) return -1;
    for (uint32_t i = 0u; i < launcher->count; ++i)
        if (launcher->items[i].enabled != 0u) ++total;
    *count = total;
    return total != 0u ? 0 : -1;
}

void sb_launcher_init(sb_launcher_t *launcher) {
    if (launcher == 0) return;
    *launcher = (sb_launcher_t){0};
}

int sb_launcher_add(sb_launcher_t *launcher, const char *id, const char *label) {
    if (launcher == 0 || !valid_string(id) || !valid_string(label) ||
        launcher->count >= SB_LAUNCHER_MAX_ITEMS || duplicate_id(launcher, id)) return -1;
    launcher->items[launcher->count] = (sb_launcher_item_t){id, label, 1u};
    if (launcher->count == 0u) launcher->selected = 0u;
    ++launcher->count;
    return 0;
}

int sb_launcher_set_open(sb_launcher_t *launcher, uint8_t open) {
    if (launcher == 0) return -1;
    launcher->open = open != 0u ? 1u : 0u;
    return 0;
}

int sb_launcher_move_selection(sb_launcher_t *launcher, int32_t delta) {
    uint32_t current;
    uint32_t enabled;
    uint32_t steps;
    uint32_t index;
    if (launcher == 0 || launcher->count == 0u || first_enabled(launcher, &index) != 0 ||
        enabled_count(launcher, &enabled) != 0) return -1;
    if (launcher->selected >= launcher->count || launcher->items[launcher->selected].enabled == 0u)
        launcher->selected = index;
    current = launcher->selected;
    if (delta == 0 || enabled == 1u) return 0;

    steps = (uint32_t)(delta < 0 ? -(int64_t)delta : (int64_t)delta) % enabled;
    for (uint32_t step = 0u; step < steps; ++step) {
        if (delta > 0) {
            index = (current + 1u) % launcher->count;
            while (launcher->items[index].enabled == 0u) index = (index + 1u) % launcher->count;
        } else {
            index = current == 0u ? launcher->count - 1u : current - 1u;
            while (launcher->items[index].enabled == 0u)
                index = index == 0u ? launcher->count - 1u : index - 1u;
        }
        current = index;
    }
    launcher->selected = current;
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
