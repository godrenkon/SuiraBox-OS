#ifndef SB_LAUNCHER_H
#define SB_LAUNCHER_H

#include <stdint.h>

#define SB_LAUNCHER_MAX_ITEMS 16u

typedef struct {
    const char *id;
    const char *label;
    uint8_t enabled;
} sb_launcher_item_t;

typedef struct {
    sb_launcher_item_t items[SB_LAUNCHER_MAX_ITEMS];
    uint32_t count;
    uint32_t selected;
    uint8_t open;
} sb_launcher_t;

void sb_launcher_init(sb_launcher_t *launcher);
int sb_launcher_add(sb_launcher_t *launcher, const char *id, const char *label);
int sb_launcher_set_open(sb_launcher_t *launcher, uint8_t open);
int sb_launcher_move_selection(sb_launcher_t *launcher, int32_t delta);
int sb_launcher_activate(sb_launcher_t *launcher, const char **out_id);
int sb_launcher_hit_test(const sb_launcher_t *launcher, int32_t x, int32_t y,
                         uint32_t left, uint32_t top, uint32_t width,
                         uint32_t row_height, uint32_t *out_index);

#endif
