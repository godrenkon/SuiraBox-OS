#include "desktop_shell.h"

void sb_desktop_shell_init(sb_desktop_shell_t *shell,
                           uint32_t screen_width, uint32_t screen_height) {
    if (shell == 0) return;
    sb_launcher_init(&shell->launcher);
    shell->screen_width = screen_width;
    shell->screen_height = screen_height;
    shell->initialized = (screen_width != 0u && screen_height != 0u) ? 1u : 0u;
}

int sb_desktop_shell_register_default_apps(sb_desktop_shell_t *shell) {
    if (shell == 0 || shell->initialized == 0u) return -1;
    if (shell->launcher.count != 0u) return -1;
    if (sb_launcher_add(&shell->launcher, "settings", "Settings") != 0) return -1;
    if (sb_launcher_add(&shell->launcher, "files", "File Manager") != 0) return -1;
    if (sb_launcher_add(&shell->launcher, "terminal", "Terminal") != 0) return -1;
    return 0;
}

int sb_desktop_shell_toggle_launcher(sb_desktop_shell_t *shell) {
    if (shell == 0 || shell->initialized == 0u) return -1;
    return sb_launcher_set_open(&shell->launcher, shell->launcher.open == 0u ? 1u : 0u);
}

int sb_desktop_shell_key(sb_desktop_shell_t *shell, uint8_t key) {
    if (shell == 0 || shell->initialized == 0u || shell->launcher.open == 0u) return -1;
    if (key == 0x48u) return sb_launcher_move_selection(&shell->launcher, -1);
    if (key == 0x50u) return sb_launcher_move_selection(&shell->launcher, 1);
    return -1;
}

int sb_desktop_shell_click(sb_desktop_shell_t *shell, int32_t x, int32_t y,
                           const char **activated_id) {
    uint32_t index;
    if (activated_id != 0) *activated_id = 0;
    if (shell == 0 || shell->initialized == 0u) return -1;

    if (x >= (int32_t)SB_SHELL_LAUNCHER_X &&
        x < (int32_t)(SB_SHELL_LAUNCHER_X + SB_SHELL_LAUNCHER_W) &&
        y >= (int32_t)(shell->screen_height - SB_GUI_TASKBAR_HEIGHT) &&
        y < (int32_t)shell->screen_height) {
        return sb_desktop_shell_toggle_launcher(shell);
    }

    if (shell->launcher.open == 0u) return -1;
    if (sb_launcher_hit_test(&shell->launcher, x, y,
                             SB_SHELL_LAUNCHER_X,
                             shell->screen_height - SB_GUI_TASKBAR_HEIGHT -
                                 SB_SHELL_MENU_ROW_H * shell->launcher.count,
                             SB_SHELL_MENU_W, SB_SHELL_MENU_ROW_H, &index) != 0) return -1;
    shell->launcher.selected = index;
    return sb_launcher_activate(&shell->launcher, activated_id);
}
