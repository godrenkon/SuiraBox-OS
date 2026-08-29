#include "settings_view.h"

static int selected_to_optional(uint32_t selected, sb_resource_optional_t *feature) {
    if (feature == 0 || selected >= SB_SETTINGS_ITEM_COUNT) return -1;
    *feature = (sb_resource_optional_t)selected;
    return 0;
}

void sb_settings_view_init(sb_settings_view_t *view, uint8_t language,
                           uint32_t optional_mask) {
    if (view == 0) return;
    sb_settings_policy_init(&view->policy, language, optional_mask);
    view->selected = 0u;
    view->remote_available = 0u;
    view->open = 1u;
}

int sb_settings_view_move(sb_settings_view_t *view, int32_t delta) {
    int64_t next;
    if (view == 0 || view->open == 0u || delta == 0) return view == 0 ? -1 : 0;
    next = (int64_t)view->selected + (int64_t)delta;
    next %= (int64_t)SB_SETTINGS_ITEM_COUNT;
    if (next < 0) next += SB_SETTINGS_ITEM_COUNT;
    view->selected = (uint32_t)next;
    return 0;
}

int sb_settings_view_toggle(sb_settings_view_t *view) {
    sb_resource_optional_t feature;
    if (view == 0 || view->open == 0u ||
        selected_to_optional(view->selected, &feature) != 0) return -1;
    return sb_settings_policy_set_optional(
        &view->policy, feature,
        (uint8_t)(sb_resource_policy_optional_enabled(
                      &view->policy.resources, (uint32_t)feature) == 0));
}

int sb_settings_view_set_remote_available(sb_settings_view_t *view,
                                          uint8_t available) {
    if (view == 0) return -1;
    view->remote_available = available != 0u ? 1u : 0u;
    return 0;
}

uint32_t sb_settings_view_mask(const sb_settings_view_t *view) {
    if (view == 0) return 0u;
    return sb_settings_policy_optional_mask(&view->policy);
}
