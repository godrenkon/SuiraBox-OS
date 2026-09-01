#ifndef SB_DESKTOP_SHELL_H
#define SB_DESKTOP_SHELL_H

#include <stdint.h>
#include "launcher.h"

#define SB_SHELL_LAUNCHER_X 18u
#define SB_SHELL_LAUNCHER_W 56u
#define SB_SHELL_LAUNCHER_H 48u
#define SB_SHELL_MENU_W 280u
#define SB_SHELL_MENU_ROW_H 44u

typedef struct {
    sb_launcher_t launcher;
    uint32_t screen_width;
    uint32_t screen_height;
    uint8_t initialized;
} sb_desktop_shell_t;

void sb_desktop_shell_init(sb_desktop_shell_t *shell,
                           uint32_t screen_width, uint32_t screen_height);
int sb_desktop_shell_register_default_apps(sb_desktop_shell_t *shell);
int sb_desktop_shell_toggle_launcher(sb_desktop_shell_t *shell);
int sb_desktop_shell_move_selection(sb_desktop_shell_t *shell, int32_t delta);
int sb_desktop_shell_activate_selected(sb_desktop_shell_t *shell, const char **activated_id);
int sb_desktop_shell_key(sb_desktop_shell_t *shell, uint8_t key);
int sb_desktop_shell_click(sb_desktop_shell_t *shell, int32_t x, int32_t y,
                           const char **activated_id);

/* Present the launcher state after compositor repainting so it is not lost on damage updates. */
void sb_desktop_shell_present_launcher(void);

#endif
