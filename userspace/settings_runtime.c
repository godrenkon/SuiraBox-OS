#include "settings_runtime.h"
#include "syscall.h"

void sb_settings_runtime_init(sb_settings_runtime_t *runtime,
                              uint8_t language, uint32_t optional_mask) {
    if (runtime == 0) return;
    sb_settings_view_init(&runtime->view, language, optional_mask);
    runtime->dirty = 0u;
    runtime->visible = 1u;
}

int sb_settings_runtime_key(sb_settings_runtime_t *runtime, uint8_t key) {
    int result;
    if (runtime == 0 || runtime->visible == 0u) return -1;
    if (key == 0x48u || key == 0x50u) {
        return sb_settings_view_move(&runtime->view, key == 0x48u ? -1 : 1);
    }
    if (key == 0x1Cu) return sb_settings_runtime_save(runtime);
    if (key == 0x39u) {
        result = sb_settings_view_toggle(&runtime->view);
        if (result == 0) runtime->dirty = 1u;
        return result;
    }
    if (key == 0x01u) return sb_settings_runtime_close(runtime);
    return -1;
}

int sb_settings_runtime_save(sb_settings_runtime_t *runtime) {
    uint64_t result;
    if (runtime == 0 || runtime->visible == 0u) return -1;
    result = sb_config_set_with_options(runtime->view.policy.language,
                                         sb_settings_view_mask(&runtime->view));
    if (result == 0u) runtime->dirty = 0u;
    return result == 0u ? 0 : -1;
}

int sb_settings_runtime_close(sb_settings_runtime_t *runtime) {
    if (runtime == 0) return -1;
    runtime->visible = 0u;
    runtime->view.open = 0u;
    return 0;
}

uint32_t sb_settings_runtime_mask(const sb_settings_runtime_t *runtime) {
    return runtime == 0 ? 0u : sb_settings_view_mask(&runtime->view);
}

uint8_t sb_settings_runtime_visible(const sb_settings_runtime_t *runtime) {
    return runtime != 0 && runtime->visible != 0u;
}
