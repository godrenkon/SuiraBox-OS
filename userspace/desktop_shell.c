#include "desktop_shell.h"
#include "gui.h"

#if __STDC_HOSTED__ == 0
#include "syscall.h"
#endif

static sb_desktop_shell_t *active_shell;

#if __STDC_HOSTED__ == 0
static const uint64_t G_J = 0x003844040404043eULL;
static const uint64_t G_P = 0x004040407c44447cULL;
static const uint64_t G_E = 0x007c40407840407cULL;
static const uint64_t G_N = 0x004242464a526242ULL;
static const uint64_t G_S = 0x007c02023c40403eULL;

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) {
    (void)sb_display_rect(x, y, w, h, rgb);
}

static void draw_glyph(uint32_t x, uint32_t y, uint64_t glyph) {
    (void)sb_display_glyph(x, y, glyph, 0xE9F2FFu);
}

static void draw_pair(uint32_t x, uint32_t y, uint64_t a, uint64_t b) {
    (void)sb_display_glyph_pair(x, y, a, b, 0xE9F2FFu);
}
#endif

static uint32_t launcher_menu_height(const sb_desktop_shell_t *shell) {
    if (shell == 0 || shell->launcher.count == 0u ||
        shell->launcher.count > UINT32_MAX / SB_SHELL_MENU_ROW_H) return 0u;
    return shell->launcher.count * SB_SHELL_MENU_ROW_H;
}

static int launcher_menu_top(const sb_desktop_shell_t *shell, int32_t *top) {
    const uint32_t menu_height = launcher_menu_height(shell);
    uint32_t taskbar_top;
    if (shell == 0 || top == 0 || menu_height == 0u ||
        shell->screen_height < SB_GUI_TASKBAR_HEIGHT || shell->screen_height > (uint32_t)INT32_MAX) return -1;
    taskbar_top = shell->screen_height - SB_GUI_TASKBAR_HEIGHT;
    if (taskbar_top < menu_height) return -1;
    *top = (int32_t)(taskbar_top - menu_height);
    return 0;
}

void sb_desktop_shell_init(sb_desktop_shell_t *shell,
                           uint32_t screen_width, uint32_t screen_height) {
    if (shell == 0) return;
    sb_launcher_init(&shell->launcher);
    shell->screen_width = screen_width;
    shell->screen_height = screen_height;
    shell->initialized = (screen_width != 0u && screen_height != 0u) ? 1u : 0u;
    active_shell = shell->initialized != 0u ? shell : 0;
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
    if (shell->launcher.open == 0u) {
        int32_t top;
        if (launcher_menu_top(shell, &top) != 0) return -1;
    }
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
    int32_t taskbar_top;
    int32_t menu_top;
    if (activated_id != 0) *activated_id = 0;
    if (shell == 0 || shell->initialized == 0u || activated_id == 0) return -1;
    if (shell->screen_height < SB_GUI_TASKBAR_HEIGHT || shell->screen_height > INT32_MAX) return -1;
    taskbar_top = (int32_t)shell->screen_height - (int32_t)SB_GUI_TASKBAR_HEIGHT;

    if (x >= (int32_t)SB_SHELL_LAUNCHER_X &&
        x < (int32_t)(SB_SHELL_LAUNCHER_X + SB_SHELL_LAUNCHER_W) &&
        y >= taskbar_top && y < (int32_t)shell->screen_height) {
        return sb_desktop_shell_toggle_launcher(shell);
    }

    if (shell->launcher.open == 0u || launcher_menu_top(shell, &menu_top) != 0) return -1;
    if (sb_launcher_hit_test(&shell->launcher, x, y,
                             SB_SHELL_LAUNCHER_X,
                             (uint32_t)menu_top,
                             SB_SHELL_MENU_W, SB_SHELL_MENU_ROW_H, &index) != 0) return -1;
    shell->launcher.selected = index;
    return sb_launcher_activate(&shell->launcher, activated_id);
}

void sb_desktop_shell_present_launcher(void) {
#if __STDC_HOSTED__ == 0
    sb_desktop_shell_t *shell = active_shell;
    int32_t top;
    const uint32_t menu_height = launcher_menu_height(shell);
    if (shell == 0 || shell->initialized == 0u || shell->launcher.open == 0u ||
        menu_height == 0u || launcher_menu_top(shell, &top) != 0) return;

    draw_rect(SB_SHELL_LAUNCHER_X, (uint32_t)top, SB_SHELL_MENU_W, menu_height, 0x171D27u);
    for (uint32_t i = 0u; i < shell->launcher.count; ++i) {
        const uint32_t row_y = (uint32_t)top + i * SB_SHELL_MENU_ROW_H;
        draw_rect(SB_SHELL_LAUNCHER_X + 2u, row_y + 2u,
                  SB_SHELL_MENU_W - 4u, SB_SHELL_MENU_ROW_H - 4u,
                  i == shell->launcher.selected ? 0x536F8Au : 0x27313Eu);
        if (i == 0u) draw_glyph(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_S);
        else if (i == 1u) draw_pair(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_E, G_N);
        else draw_pair(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_J, G_P);
    }
#else
    (void)active_shell;
#endif
}
