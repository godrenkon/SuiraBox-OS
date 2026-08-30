#include "../syscall.h"

static const uint64_t SETTINGS_ICON = 0x003C427E625A423Cu;
static const uint64_t G_J = 0x003844040404043eULL;
static const uint64_t G_P = 0x004040407c44447cULL;
static const uint64_t G_E = 0x007c40407840407cULL;
static const uint64_t G_N = 0x004242464a526242ULL;
static const uint64_t G_Z = 0x007e20100804027eULL;
static const uint64_t G_H = 0x007e404040407e40ULL;
static const uint64_t G_S = 0x007c02023c40403eULL;

static void draw_pair(uint32_t x, uint32_t y, uint64_t a, uint64_t b, uint32_t rgb) {
    (void)sb_display_glyph_pair(x, y, a, b, rgb);
}

static void draw_language(uint32_t index, uint32_t y, uint32_t selected) {
    const uint32_t rgb = index == selected ? 0xFFFFFFu : 0xB8C4D4u;
    if (index == 0u) draw_pair(72u, y, G_J, G_P, rgb);
    else if (index == 1u) draw_pair(72u, y, G_E, G_N, rgb);
    else if (index == 2u) draw_pair(72u, y, G_Z, G_H, rgb);
    else draw_pair(72u, y, G_E, G_S, rgb);
    (void)sb_display_rect(48u, y - 8u, 192u, 40u, index == selected ? 0x536F8Au : 0x27313Eu);
    if (index == selected) draw_pair(72u, y, index == 0u ? G_J : index == 1u ? G_E : index == 2u ? G_Z : G_E,
                                     index == 0u ? G_P : index == 1u ? G_N : index == 2u ? G_H : G_S, 0xFFFFFFu);
}

static uint32_t current_language(void) {
    const uint64_t value = sb_config_get();
    const uint32_t language = (uint32_t)((value >> 8) & 0xFFu);
    return language <= 3u ? language : 1u;
}

uint64_t sb_app_main(void) {
    uint32_t selected = current_language();

    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 24u, SETTINGS_ICON, 0xE9F2FFu);
    (void)sb_display_rect(48u, 96u, 192u, 176u, 0x202A38u);
    for (uint32_t i = 0u; i < 4u; ++i) draw_language(i, 120u + i * 40u, selected);

    for (;;) {
        const uint64_t key = sb_input_key();
        if (key == 0u) {
            (void)sb_yield();
            continue;
        }
        if ((key & 0x80u) != 0u) continue;

        if (key == 0x48u && selected > 0u) {
            --selected;
        } else if (key == 0x50u && selected < 3u) {
            ++selected;
        } else if (key == 0x1Cu) {
            const uint64_t result = sb_config_set(selected);
            if (result == 0u) {
                (void)sb_display_rect(48u, 288u, 192u, 40u, 0x2F6F55u);
                (void)sb_display_glyph(96u, 300u, SETTINGS_ICON, 0xFFFFFFu);
            }
        } else if (key == 0x01u) {
            return 0u;
        }

        (void)sb_display_rect(48u, 96u, 192u, 176u, 0x202A38u);
        for (uint32_t i = 0u; i < 4u; ++i) draw_language(i, 120u + i * 40u, selected);
    }
}
