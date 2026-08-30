#include <stdint.h>
#include <limits.h>

#include "gui.h"
#include "syscall.h"
#include "compositor.h"
#include "surface.h"
#include "event_queue.h"
#include "config.h"
#include "desktop_shell.h"

static const uint64_t G_J = 0x003844040404043eULL;
static const uint64_t G_P = 0x004040407c44447cULL;
static const uint64_t G_E = 0x007c40407840407cULL;
static const uint64_t G_N = 0x004242464a526242ULL;
static const uint64_t G_Z = 0x007e20100804027eULL;
static const uint64_t G_H = 0x007e404040407e40ULL;
static const uint64_t G_S = 0x007c02023c40403eULL;
static int32_t g_cursor_x;
static int32_t g_cursor_y;

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) { (void)sb_display_rect(x, y, w, h, rgb); }
static void draw_glyph(uint32_t x, uint32_t y, uint64_t glyph) { (void)sb_display_glyph(x, y, glyph, 0xE9F2FFu); }
static void draw_pair(uint32_t x, uint32_t y, uint64_t a, uint64_t b) { (void)sb_display_glyph_pair(x, y, a, b, 0xE9F2FFu); }

static void draw_first_boot(uint32_t width, uint32_t height, uint32_t selection, uint8_t open) {
    const uint32_t mx = (width - 560u) / 2u;
    const uint32_t my = (height - 360u) / 2u;
    const uint32_t sx = (width - 420u) / 2u;
    draw_rect(0u, 0u, width, height, 0x0C1018u);
    draw_rect(0u, 0u, width, 72u, 0x16202Cu);
    draw_rect(0u, height - 72u, width, 72u, 0x16202Cu);
    draw_rect(mx, my, 560u, 360u, 0x313B4Au);
    draw_rect(mx + 24u, my + 24u, 512u, 52u, 0x3A485Au);
    draw_pair(mx + 36u, my + 38u, G_J, G_P);
    draw_rect(sx, my + 104u, 420u, 52u, 0x202A38u);
    draw_rect(sx + 356u, my + 112u, 52u, 36u, 0x3A485Au);
    if (selection == 0u) draw_pair(sx + 20u, my + 124u, G_J, G_P);
    else if (selection == 1u) draw_pair(sx + 20u, my + 124u, G_E, G_N);
    else if (selection == 2u) draw_pair(sx + 20u, my + 124u, G_Z, G_H);
    else draw_pair(sx + 20u, my + 124u, G_E, G_S);
    if (open) {
        draw_rect(sx, my + 158u, 420u, 176u, 0x171D27u);
        for (uint32_t i = 0u; i < 4u; ++i) draw_rect(sx + 2u, my + 160u + i * 44u, 416u, 40u, 0x27313Eu);
        draw_rect(sx + 2u, my + 160u + selection * 44u, 416u, 40u, 0x536F8Au);
        draw_pair(sx + 20u, my + 176u, G_J, G_P);
        draw_pair(sx + 20u, my + 220u, G_E, G_N);
        draw_pair(sx + 20u, my + 264u, G_Z, G_H);
        draw_pair(sx + 20u, my + 308u, G_E, G_S);
    }
    draw_rect((width - 240u) / 2u, my + 300u, 240u, 44u, 0x536F8Au);
}

static void draw_launcher(const sb_desktop_shell_t *shell) {
    uint32_t top;
    if (shell == 0 || shell->launcher.open == 0u || shell->screen_height < SB_GUI_TASKBAR_HEIGHT) return;
    if (shell->launcher.count > UINT32_MAX / SB_SHELL_MENU_ROW_H) return;
    const uint32_t menu_height = SB_SHELL_MENU_ROW_H * shell->launcher.count;
    const uint32_t taskbar_top = shell->screen_height - SB_GUI_TASKBAR_HEIGHT;
    if (taskbar_top < menu_height) return;
    top = taskbar_top - menu_height;
    draw_rect(SB_SHELL_LAUNCHER_X, top, SB_SHELL_MENU_W, menu_height, 0x171D27u);
    for (uint32_t i = 0u; i < shell->launcher.count; ++i) {
        const uint32_t row_y = top + i * SB_SHELL_MENU_ROW_H;
        draw_rect(SB_SHELL_LAUNCHER_X + 2u, row_y + 2u, SB_SHELL_MENU_W - 4u, SB_SHELL_MENU_ROW_H - 4u,
                  i == shell->launcher.selected ? 0x536F8Au : 0x27313Eu);
        if (i == 0u) draw_glyph(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_S);
        else if (i == 1u) draw_pair(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_E, G_N);
        else draw_pair(SB_SHELL_LAUNCHER_X + 18u, row_y + 14u, G_J, G_P);
    }
}

static int point_inside(int32_t x, int32_t y, int32_t left, int32_t top, uint32_t width, uint32_t height) {
    const int64_t rx = (int64_t)left + (int64_t)width;
    const int64_t by = (int64_t)top + (int64_t)height;
    return (int64_t)x >= (int64_t)left && (int64_t)y >= (int64_t)top && (int64_t)x < rx && (int64_t)y < by;
}

static int commit_language(sb_config_record_t *config, uint32_t selection) {
    if (config == 0 || selection > (uint32_t)SB_LANGUAGE_SPANISH) return -1;
    if (sb_config_make(config, (sb_language_t)selection, 1u) != 0) return -1;
    return sb_config_validate(config);
}

static int persist_language(sb_config_record_t *config, uint32_t selection) {
    if (commit_language(config, selection) != 0) return -1;
    {
        const uint64_t result = sb_config_set(selection);
        return result == 0u ? 0 : -1;
    }
}

static int load_persisted_language(sb_config_record_t *config, uint32_t *selection) {
    const uint64_t persisted = sb_config_get();
    const uint32_t language = (uint32_t)((persisted >> 8) & 0xFFu);
    if ((persisted & 1u) == 0u || language > (uint32_t)SB_LANGUAGE_SPANISH) return 0;
    if (selection == 0 || commit_language(config, language) != 0) return -1;
    *selection = language;
    return 1;
}

static void decode_mouse(uint64_t packet, int32_t *dx, int32_t *dy, uint8_t *buttons) {
    *dx = (int32_t)(int8_t)((packet >> 16) & 0xFFu);
    *dy = (int32_t)(int8_t)((packet >> 24) & 0xFFu);
    *buttons = (uint8_t)((packet >> 8) & 0x07u);
}

static void mark_damage(sb_surface_t *surface, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (surface != 0 && width != 0u && height != 0u) (void)sb_surface_damage_rect(surface, x, y, width, height);
}

static void mark_taskbar_damage(sb_surface_t *surface, uint32_t width, uint32_t height) {
    if (height >= SB_GUI_TASKBAR_HEIGHT) mark_damage(surface, 0, (int32_t)height - (int32_t)SB_GUI_TASKBAR_HEIGHT, width, SB_GUI_TASKBAR_HEIGHT);
}

static void present_damage(sb_surface_t *surface, uint32_t width, uint32_t height, sb_gui_window_manager_t *wm,
                           sb_surface_rect_t *damage, uint32_t capacity) {
    uint32_t count = 0u;
    uint8_t full = 0u;
    sb_compositor_style_t style;
    if (sb_surface_take_damage(surface, damage, capacity, &count, &full) != 0) return;
    if (full == 0u && count == 0u) return;
    sb_compositor_init(&style, width, height);
    sb_compositor_present_damage(&style, wm, damage, count, full);
    sb_compositor_present_cursor(&style, g_cursor_x, g_cursor_y);
    sb_desktop_shell_present_launcher();
}

static int shell_open_app(sb_gui_window_manager_t *wm, const char *id, uint32_t width, uint32_t height) {
    int32_t x;
    int32_t y;
    if (wm == 0 || id == 0 || width < SB_GUI_MIN_WINDOW_WIDTH || height < SB_GUI_MIN_WINDOW_HEIGHT) return -1;
    if (id[0] == 's') { x = 170; y = 120; }
    else if (id[0] == 'f') { x = 230; y = 150; }
    else { x = 290; y = 180; }
    return sb_gui_create_window(wm, x, y, width, height) != 0 ? 0 : -1;
}

void sb_desktop_main(void) {
    const uint64_t display = sb_display_info();
    const uint32_t width = (uint32_t)(display >> 32);
    const uint32_t height = (uint32_t)((display >> 16) & 0xFFFFu);
    uint32_t selection = (uint32_t)SB_LANGUAGE_ENGLISH;
    uint8_t first_boot = 1u;
    uint8_t dropdown_open = 0u;
    uint8_t last_buttons = 0u;
    uint8_t dragging = 0u;
    sb_gui_resize_edge_t resizing = SB_GUI_RESIZE_NONE;
    int32_t drag_dx = 0;
    int32_t drag_dy = 0;
    sb_gui_window_manager_t wm;
    sb_gui_event_queue_t events;
    sb_surface_t surface;
    sb_surface_rect_t damage[SB_SURFACE_MAX_DAMAGE];
    sb_config_record_t config;
    sb_gui_window_t *main_window;
    sb_desktop_shell_t shell;

    if (width == 0u || height == 0u || width > UINT32_MAX / 4u) for (;;) { }
    if (sb_surface_init(&surface, width, height, width * 4u, SB_SURFACE_FORMAT_XRGB8888, 0) != 0) for (;;) { }
    sb_gui_init(&wm);
    sb_gui_event_queue_init(&events);
    sb_desktop_shell_init(&shell, width, height);
    if (sb_desktop_shell_register_default_apps(&shell) != 0) for (;;) { }
    main_window = sb_gui_create_window(&wm, 252, 150, 520u, 320u);
    if (main_window == 0) for (;;) { }
    main_window->visible = 0u;

    {
        const int persisted = load_persisted_language(&config, &selection);
        if (persisted < 0) for (;;) { }
        if (persisted > 0) {
            first_boot = 0u;
            main_window->visible = 1u;
        } else if (commit_language(&config, selection) != 0) {
            for (;;) { }
        }
    }

    g_cursor_x = (int32_t)(width / 2u);
    g_cursor_y = (int32_t)(height / 2u);
    sb_surface_damage_all(&surface);
    if (first_boot != 0u) draw_first_boot(width, height, selection, dropdown_open);
    else present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);

    for (;;) {
        const uint64_t key = sb_input_key();
        if (key != 0u && (key & 0x80u) == 0u) {
            sb_gui_event_t event = {SB_GUI_EVENT_KEY, g_cursor_x, g_cursor_y, 0, 0, 0, (uint8_t)key};
            if (sb_gui_event_queue_push(&events, &event) == 0) {
                while (sb_gui_event_queue_pop(&events, &event) == 0) {
                    if (first_boot != 0u) {
                        if (event.key == 0x39u) {
                            dropdown_open = dropdown_open == 0u ? 1u : 0u;
                            draw_first_boot(width, height, selection, dropdown_open);
                        } else if (dropdown_open != 0u && event.key == 0x48u && selection > 0u) {
                            --selection;
                            (void)commit_language(&config, selection);
                            draw_first_boot(width, height, selection, dropdown_open);
                        } else if (dropdown_open != 0u && event.key == 0x50u && selection < 3u) {
                            ++selection;
                            (void)commit_language(&config, selection);
                            draw_first_boot(width, height, selection, dropdown_open);
                        } else if (event.key == 0x1Cu) {
                            if (dropdown_open != 0u) {
                                dropdown_open = 0u;
                                draw_first_boot(width, height, selection, dropdown_open);
                            } else if (persist_language(&config, selection) == 0) {
                                first_boot = 0u;
                                main_window->visible = 1u;
                                sb_surface_damage_all(&surface);
                                present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                            }
                        }
                        continue;
                    }

                    if (event.key == 0x48u || event.key == 0x50u) {
                        (void)sb_desktop_shell_key(&shell, event.key);
                        sb_surface_damage_all(&surface);
                        present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                        if (shell.launcher.open != 0u) draw_launcher(&shell);
                    }
                }
            }
        }

        const uint64_t packet = sb_input_mouse();
        if (packet == 0u) continue;
        int32_t dx;
        int32_t dy;
        uint8_t buttons;
        decode_mouse(packet, &dx, &dy, &buttons);
        sb_gui_event_t mouse = {((buttons ^ last_buttons) & 0x07u) != 0u ? SB_GUI_EVENT_MOUSE_BUTTON : SB_GUI_EVENT_MOUSE_MOVE,
                                g_cursor_x, g_cursor_y, (int16_t)dx, (int16_t)dy, buttons, 0};
        if (sb_gui_event_queue_push(&events, &mouse) != 0) continue;
        while (sb_gui_event_queue_pop(&events, &mouse) == 0) {
            const int32_t old_cursor_x = g_cursor_x;
            const int32_t old_cursor_y = g_cursor_y;
            g_cursor_x += mouse.dx; g_cursor_y -= mouse.dy;
            if (g_cursor_x < 0) {
                g_cursor_x = 0;
            }
            if (g_cursor_y < 0) {
                g_cursor_y = 0;
            }
            if (g_cursor_x >= (int32_t)width) g_cursor_x = (int32_t)width - 1;
            if (g_cursor_y >= (int32_t)height) g_cursor_y = (int32_t)height - 1;
            mark_damage(&surface, old_cursor_x, old_cursor_y, 8u, 8u); mark_damage(&surface, g_cursor_x, g_cursor_y, 8u, 8u);

            if (mouse.type == SB_GUI_EVENT_MOUSE_BUTTON && (mouse.buttons & 1u) != 0u && (last_buttons & 1u) == 0u) {
                if (first_boot != 0u) {
                    const uint32_t modal_y = (height - 360u) / 2u;
                    const uint32_t field_x = (width - 420u) / 2u;
                    if (point_inside(g_cursor_x, g_cursor_y, (int32_t)field_x, (int32_t)(modal_y + 104u), 420u, 52u)) {
                        dropdown_open = dropdown_open == 0u ? 1u : 0u; draw_first_boot(width, height, selection, dropdown_open);
                    } else if (dropdown_open != 0u && point_inside(g_cursor_x, g_cursor_y, (int32_t)field_x, (int32_t)(modal_y + 160u), 420u, 176u)) {
                        const uint32_t row = (uint32_t)(g_cursor_y - (int32_t)(modal_y + 160u)) / 44u;
                        if (row < 4u && commit_language(&config, row) == 0) selection = row;
                        dropdown_open = 0u; draw_first_boot(width, height, selection, dropdown_open);
                    } else if (dropdown_open == 0u && point_inside(g_cursor_x, g_cursor_y, (int32_t)((width - 240u) / 2u),
                                                                     (int32_t)(modal_y + 300u), 240u, 44u) &&
                               persist_language(&config, selection) == 0) {
                        first_boot = 0u; main_window->visible = 1u; sb_surface_damage_all(&surface);
                        present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                    } else if (dropdown_open != 0u) { dropdown_open = 0u; draw_first_boot(width, height, selection, dropdown_open); }
                } else {
                    const char *activated_id = 0;
                    if (sb_desktop_shell_click(&shell, g_cursor_x, g_cursor_y, &activated_id) == 0) {
                        sb_surface_damage_all(&surface); present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                        if (activated_id != 0 && shell_open_app(&wm, activated_id, 420u, 260u) == 0) {
                            sb_surface_damage_all(&surface); present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                        }
                        if (shell.launcher.open != 0u) draw_launcher(&shell);
                    } else {
                        const uint32_t taskbar_id = sb_gui_hit_taskbar(&wm, g_cursor_x, g_cursor_y, width, height);
                        if (taskbar_id != 0u) {
                            sb_gui_window_t *taskbar_window = sb_gui_find_window(&wm, taskbar_id);
                            if (taskbar_window != 0 && taskbar_window->minimized != 0u) {
                                mark_taskbar_damage(&surface, width, height);
                                if (sb_gui_restore_window(&wm, taskbar_id) == 0) {
                                    sb_gui_window_t *restored = sb_gui_find_window(&wm, taskbar_id);
                                    if (restored != 0) mark_damage(&surface, restored->x, restored->y, restored->width, restored->height);
                                    present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                                }
                                dragging = 0u; resizing = SB_GUI_RESIZE_NONE;
                            }
                        } else {
                            sb_gui_window_t *hit = sb_gui_hit_test(&wm, g_cursor_x, g_cursor_y);
                            if (hit != 0) {
                                const uint32_t id = hit->id;
                                const int32_t wx = hit->x; const int32_t wy = hit->y;
                                const uint32_t ww = hit->width; const uint32_t wh = hit->height;
                                const sb_gui_control_t control = sb_gui_hit_control(hit, g_cursor_x, g_cursor_y);
                                const sb_gui_resize_edge_t edge = sb_gui_hit_resize(hit, g_cursor_x, g_cursor_y);
                                (void)sb_gui_focus_window(&wm, id);
                                if (control == SB_GUI_CONTROL_CLOSE) { mark_damage(&surface, wx, wy, ww, wh); (void)sb_gui_destroy_window(&wm, id); dragging = 0u; resizing = SB_GUI_RESIZE_NONE; }
                                else if (control == SB_GUI_CONTROL_MINIMIZE) { mark_damage(&surface, wx, wy, ww, wh); mark_taskbar_damage(&surface, width, height); (void)sb_gui_set_minimized(&wm, id, 1u); dragging = 0u; resizing = SB_GUI_RESIZE_NONE; }
                                else if (control == SB_GUI_CONTROL_MAXIMIZE) { mark_damage(&surface, wx, wy, ww, wh); (void)sb_gui_set_maximized(&wm, id, hit->maximized == 0u ? 1u : 0u, width, height); dragging = 0u; resizing = SB_GUI_RESIZE_NONE; }
                                else if (edge != SB_GUI_RESIZE_NONE) { resizing = edge; dragging = 0u; drag_dx = g_cursor_x - wx; drag_dy = g_cursor_y - wy; }
                                else if (g_cursor_y < wy + (int32_t)SB_GUI_TITLEBAR_HEIGHT) { resizing = SB_GUI_RESIZE_NONE; dragging = 1u; drag_dx = g_cursor_x - wx; drag_dy = g_cursor_y - wy; }
                                else { dragging = 0u; resizing = SB_GUI_RESIZE_NONE; }
                                present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                            }
                        }
                    }
                }
            }

            if (first_boot == 0u && (mouse.buttons & 1u) != 0u) {
                sb_gui_window_t *focused = sb_gui_find_window(&wm, wm.focused_id);
                if (focused != 0 && focused->visible != 0u && focused->minimized == 0u) {
                    const int32_t old_x = focused->x; const int32_t old_y = focused->y;
                    const uint32_t old_width = focused->width; const uint32_t old_height = focused->height;
                    if (resizing != SB_GUI_RESIZE_NONE) {
                        uint32_t new_width = focused->width; uint32_t new_height = focused->height;
                        if (resizing == SB_GUI_RESIZE_RIGHT || resizing == SB_GUI_RESIZE_BOTTOM_RIGHT) {
                            int64_t candidate = (int64_t)g_cursor_x - (int64_t)focused->x;
                            if (candidate < 0) {
                                candidate = 0;
                            }
                            if (candidate > (int64_t)UINT32_MAX) {
                                candidate = (int64_t)UINT32_MAX;
                            }
                            new_width = (uint32_t)candidate;
                        }
                        if (resizing == SB_GUI_RESIZE_BOTTOM || resizing == SB_GUI_RESIZE_BOTTOM_RIGHT) {
                            int64_t candidate = (int64_t)g_cursor_y - (int64_t)focused->y;
                            if (candidate < 0) {
                                candidate = 0;
                            }
                            if (candidate > (int64_t)UINT32_MAX) {
                                candidate = (int64_t)UINT32_MAX;
                            }
                            new_height = (uint32_t)candidate;
                        }
                        if (sb_gui_resize_window(&wm, focused->id, new_width, new_height) == 0) {
                            mark_damage(&surface, old_x, old_y, old_width, old_height);
                            mark_damage(&surface, focused->x, focused->y, focused->width, focused->height);
                            present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                        }
                    } else if (dragging && focused->maximized == 0u) {
                        focused->x = g_cursor_x - drag_dx; focused->y = g_cursor_y - drag_dy;
                        if (focused->x < 0) {
                            focused->x = 0;
                        }
                        if (focused->y < (int32_t)SB_GUI_TITLEBAR_HEIGHT) {
                            focused->y = SB_GUI_TITLEBAR_HEIGHT;
                        }
                        mark_damage(&surface, old_x, old_y, old_width, old_height); mark_damage(&surface, focused->x, focused->y, focused->width, focused->height);
                        present_damage(&surface, width, height, &wm, damage, SB_SURFACE_MAX_DAMAGE);
                    }
                }
            }

            if ((mouse.buttons & 1u) == 0u) { dragging = 0u; resizing = SB_GUI_RESIZE_NONE; }
            last_buttons = mouse.buttons;
        }
    }
}
