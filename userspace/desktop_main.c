#include <stdint.h>
#include <limits.h>

#include "gui.h"
#include "syscall.h"
#include "compositor.h"
#include "surface.h"
#include "event_queue.h"
#include "config.h"

static const uint64_t G_J = 0x003844040404043eULL;
static const uint64_t G_P = 0x004040407c44447cULL;
static const uint64_t G_E = 0x007c40407840407cULL;
static const uint64_t G_N = 0x004242464a526242ULL;
static const uint64_t G_Z = 0x007e20100804027eULL;
static const uint64_t G_H = 0x007e404040407e40ULL;
static const uint64_t G_S = 0x007c02023c40403eULL;

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t rgb) {
    (void)sb_display_rect(x, y, w, h, rgb);
}

static void draw_glyph(uint32_t x, uint32_t y, uint64_t glyph) {
    (void)sb_display_glyph(x, y, glyph, 0xE9F2FFu);
}

static void draw_pair(uint32_t x, uint32_t y, uint64_t a, uint64_t b) {
    draw_glyph(x, y, a);
    draw_glyph(x + 10u, y, b);
}

static void screen_background(uint32_t width, uint32_t height) {
    draw_rect(0u, 0u, width, height, 0x0C1018u);
    draw_rect(0u, 0u, width, 72u, 0x16202Cu);
    draw_rect(0u, height - 72u, width, 72u, 0x16202Cu);
}

static void draw_select(uint32_t width, uint32_t modal_y, uint32_t selection, int open) {
    uint32_t x = (width - 420u) / 2u;
    draw_rect(x, modal_y + 104u, 420u, 52u, 0x202A38u);
    draw_rect(x + 356u, modal_y + 112u, 52u, 36u, 0x3A485Au);
    if (selection == 0u) draw_pair(x + 20u, modal_y + 124u, G_J, G_P);
    else if (selection == 1u) draw_pair(x + 20u, modal_y + 124u, G_E, G_N);
    else if (selection == 2u) draw_pair(x + 20u, modal_y + 124u, G_Z, G_H);
    else draw_pair(x + 20u, modal_y + 124u, G_E, G_S);
    if (!open) return;
    draw_rect(x, modal_y + 158u, 420u, 176u, 0x171D27u);
    for (uint32_t i = 0u; i < 4u; ++i)
        draw_rect(x + 2u, modal_y + 160u + i * 44u, 416u, 40u, 0x27313Eu);
    draw_rect(x + 2u, modal_y + 160u + selection * 44u, 416u, 40u, 0x536F8Au);
    draw_pair(x + 20u, modal_y + 176u, G_J, G_P);
    draw_pair(x + 20u, modal_y + 220u, G_E, G_N);
    draw_pair(x + 20u, modal_y + 264u, G_Z, G_H);
    draw_pair(x + 20u, modal_y + 308u, G_E, G_S);
}

static void draw_first_boot(uint32_t width, uint32_t height, uint32_t selection, int open) {
    const uint32_t modal_x = (width - 560u) / 2u;
    const uint32_t modal_y = (height - 360u) / 2u;
    screen_background(width, height);
    draw_rect(modal_x, modal_y, 560u, 360u, 0x313B4Au);
    draw_rect(modal_x + 24u, modal_y + 24u, 512u, 52u, 0x3A485Au);
    draw_pair(modal_x + 36u, modal_y + 38u, G_J, G_P);
    draw_select(width, modal_y, selection, open);
    draw_rect((width - 240u) / 2u, modal_y + 300u, 240u, 44u, 0x536F8Au);
}

static void draw_desktop_damage(uint32_t width, uint32_t height,
                                sb_gui_window_manager_t *wm,
                                const sb_surface_rect_t *damage,
                                uint32_t damage_count, uint8_t full_damage) {
    sb_compositor_style_t style;
    sb_compositor_init(&style, width, height);
    sb_compositor_present_damage(&style, wm, damage, damage_count, full_damage);
}

static int point_inside(int32_t x, int32_t y, int32_t left, int32_t top,
                        uint32_t width, uint32_t height) {
    if (x < left || y < top) return 0;
    if ((uint32_t)(x - left) >= width) return 0;
    if ((uint32_t)(y - top) >= height) return 0;
    return 1;
}

static void decode_mouse(uint64_t packet, int32_t *dx, int32_t *dy, uint8_t *buttons) {
    *dx = (int32_t)(int8_t)((packet >> 16) & 0xFFu);
    *dy = (int32_t)(int8_t)((packet >> 24) & 0xFFu);
    *buttons = (uint8_t)((packet >> 8) & 0x07u);
}

static int commit_language(sb_config_record_t *config, uint32_t selection) {
    if (config == 0 || selection > (uint32_t)SB_LANGUAGE_SPANISH) return -1;
    if (sb_config_make(config, (sb_language_t)selection, 1u) != 0) return -1;
    return sb_config_validate(config);
}

static void present_damage(sb_surface_t *surface, uint32_t width, uint32_t height,
                           sb_gui_window_manager_t *wm,
                           sb_surface_rect_t *damage, uint32_t capacity) {
    uint32_t count = 0u;
    uint8_t full = 0u;
    if (sb_surface_take_damage(surface, damage, capacity, &count, &full) != 0) return;
    if (full == 0u && count == 0u) return;
    draw_desktop_damage(width, height, wm, damage, count, full);
}

void sb_desktop_main(void) {
    uint64_t display = sb_display_info();
    uint32_t width = (uint32_t)(display >> 32);
    uint32_t height = (uint32_t)((display >> 16) & 0xFFFFu);
    uint32_t selection = 1u;
    int first_boot = 1;
    int dropdown_open = 0;
    int32_t cursor_x = (int32_t)(width / 2u);
    int32_t cursor_y = (int32_t)(height / 2u);
    uint8_t last_buttons = 0u;
    int dragging = 0;
    int32_t drag_dx = 0;
    int32_t drag_dy = 0;
    sb_gui_window_manager_t wm;
    sb_gui_event_queue_t event_queue;
    sb_surface_t frame_surface;
    sb_surface_rect_t damage[SB_SURFACE_MAX_DAMAGE];
    sb_config_record_t config;
    uint32_t zero_count = 0u;
    uint8_t zero_full = 0u;

    if (width == 0u || height == 0u || width > UINT32_MAX / 4u) for (;;) { }
    if (sb_surface_init(&frame_surface, width, height, width * 4u,
                        SB_SURFACE_FORMAT_XRGB8888, 0) != 0) for (;;) { }
    sb_gui_event_queue_init(&event_queue);
    sb_gui_init(&wm);
    sb_gui_window_t *main_window = sb_gui_create_window(&wm, 252, 150, 520u, 320u);
    if (main_window != 0) main_window->visible = 0u;
    if (commit_language(&config, selection) != 0) for (;;) { }

    sb_surface_damage_all(&frame_surface);
    draw_first_boot(width, height, selection, dropdown_open);
    (void)sb_surface_take_damage(&frame_surface, damage, SB_SURFACE_MAX_DAMAGE,
                                  &zero_count, &zero_full);

    for (;;) {
        uint64_t key = sb_input_key();
        if (key != 0u && (key & 0x80u) == 0u) {
            sb_gui_event_t event = {SB_GUI_EVENT_KEY, cursor_x, cursor_y, 0, 0, 0,
                                    (uint8_t)key};
            if (sb_gui_event_queue_push(&event_queue, &event) == 0) {
                while (sb_gui_event_queue_pop(&event_queue, &event) == 0) {
                    key = event.key;
                    if (!first_boot) break;
                    if (key == 0x39u) {
                        dropdown_open = !dropdown_open;
                        draw_first_boot(width, height, selection, dropdown_open);
                    } else if (dropdown_open && key == 0x48u && selection > 0u) {
                        --selection;
                        (void)commit_language(&config, selection);
                        draw_first_boot(width, height, selection, dropdown_open);
                    } else if (dropdown_open && key == 0x50u && selection < 3u) {
                        ++selection;
                        (void)commit_language(&config, selection);
                        draw_first_boot(width, height, selection, dropdown_open);
                    } else if (key == 0x1Cu) {
                        if (dropdown_open) {
                            dropdown_open = 0;
                            draw_first_boot(width, height, selection, dropdown_open);
                        } else if (commit_language(&config, selection) == 0) {
                            first_boot = 0;
                            if (main_window != 0) main_window->visible = 1u;
                            sb_surface_damage_all(&frame_surface);
                            present_damage(&frame_surface, width, height, &wm,
                                           damage, SB_SURFACE_MAX_DAMAGE);
                        }
                    }
                }
            }
        }

        uint64_t packet = sb_input_mouse();
        if (packet == 0u) continue;
        int32_t dx;
        int32_t dy;
        uint8_t buttons;
        decode_mouse(packet, &dx, &dy, &buttons);
        sb_gui_event_type_t type = ((buttons ^ last_buttons) & 0x07u) != 0u
                                 ? SB_GUI_EVENT_MOUSE_BUTTON
                                 : SB_GUI_EVENT_MOUSE_MOVE;
        sb_gui_event_t event = {type, cursor_x, cursor_y,
                                (int16_t)dx, (int16_t)dy, buttons, 0};
        if (sb_gui_event_queue_push(&event_queue, &event) != 0) continue;
        while (sb_gui_event_queue_pop(&event_queue, &event) == 0) {
            cursor_x += event.dx;
            cursor_y -= event.dy;
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_x >= (int32_t)width) cursor_x = (int32_t)width - 1;
            if (cursor_y >= (int32_t)height) cursor_y = (int32_t)height - 1;

            if (event.type == SB_GUI_EVENT_MOUSE_BUTTON &&
                (event.buttons & 1u) != 0u && (last_buttons & 1u) == 0u) {
                if (first_boot) {
                    uint32_t modal_y = (height - 360u) / 2u;
                    uint32_t field_x = (width - 420u) / 2u;
                    if (point_inside(cursor_x, cursor_y, (int32_t)field_x,
                                     (int32_t)(modal_y + 104u), 420u, 52u)) {
                        dropdown_open = !dropdown_open;
                        draw_first_boot(width, height, selection, dropdown_open);
                    } else if (dropdown_open && cursor_x >= (int32_t)field_x &&
                               cursor_x < (int32_t)(field_x + 420u) &&
                               cursor_y >= (int32_t)(modal_y + 160u) &&
                               cursor_y < (int32_t)(modal_y + 336u)) {
                        uint32_t row = (uint32_t)(cursor_y - (int32_t)(modal_y + 160u)) / 44u;
                        if (row < 4u && commit_language(&config, row) == 0) selection = row;
                        dropdown_open = 0;
                        draw_first_boot(width, height, selection, dropdown_open);
                    } else if (!dropdown_open && point_inside(cursor_x, cursor_y,
                               (int32_t)((width - 240u) / 2u),
                               (int32_t)(modal_y + 300u), 240u, 44u) &&
                               commit_language(&config, selection) == 0) {
                        first_boot = 0;
                        if (main_window != 0) main_window->visible = 1u;
                        sb_surface_damage_all(&frame_surface);
                        present_damage(&frame_surface, width, height, &wm,
                                       damage, SB_SURFACE_MAX_DAMAGE);
                    } else if (dropdown_open) {
                        dropdown_open = 0;
                        draw_first_boot(width, height, selection, dropdown_open);
                    }
                } else {
                    sb_gui_window_t *hit = sb_gui_hit_test(&wm, cursor_x, cursor_y);
                    if (hit != 0) {
                        const uint32_t hit_id = hit->id;
                        const int32_t hit_x = hit->x;
                        const int32_t hit_y = hit->y;
                        (void)sb_gui_focus_window(&wm, hit_id);
                        dragging = 1;
                        drag_dx = cursor_x - hit_x;
                        drag_dy = cursor_y - hit_y;
                    }
                }
            }

            if (!first_boot && (event.buttons & 1u) != 0u && dragging) {
                sb_gui_window_t *focused = sb_gui_find_window(&wm, wm.focused_id);
                if (focused != 0) {
                    int32_t old_x = focused->x;
                    int32_t old_y = focused->y;
                    uint32_t old_width = focused->width;
                    uint32_t old_height = focused->height;
                    focused->x = cursor_x - drag_dx;
                    focused->y = cursor_y - drag_dy;
                    if (focused->x < 0) focused->x = 0;
                    if (focused->y < 72) focused->y = 72;
                    (void)sb_surface_damage_rect(&frame_surface, old_x, old_y,
                                                  old_width, old_height);
                    (void)sb_surface_damage_rect(&frame_surface, focused->x, focused->y,
                                                  focused->width, focused->height);
                    present_damage(&frame_surface, width, height, &wm,
                                   damage, SB_SURFACE_MAX_DAMAGE);
                }
            }

            if ((event.buttons & 1u) == 0u) dragging = 0;
            last_buttons = event.buttons;
        }
    }
}