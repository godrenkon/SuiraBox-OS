#include "../syscall.h"

static const uint64_t FILES_ICON = 0x007E42427840407Eu;
static const uint64_t FILE_ICON = 0x007E424242427E00u;
static const uint64_t G_0 = 0x003C666E76663C00u;
static const uint64_t G_1 = 0x0018381818187E00u;
static const uint64_t G_2 = 0x003C66061C307E00u;
static const uint64_t G_3 = 0x003C66061C663C00u;
static const uint64_t G_4 = 0x000C1C3C6C7E0C00u;
static const uint64_t G_5 = 0x007E607C06663C00u;
static const uint64_t G_6 = 0x001C307C66663C00u;
static const uint64_t G_7 = 0x007E060C18303000u;
static const uint64_t G_8 = 0x003C663C66663C00u;
static const uint64_t G_9 = 0x003C66663E063C00u;

static uint64_t digit_glyph(uint32_t digit) {
    static const uint64_t glyphs[10] = {G_0, G_1, G_2, G_3, G_4, G_5, G_6, G_7, G_8, G_9};
    return digit < 10u ? glyphs[digit] : G_0;
}

static void draw_u32(uint32_t x, uint32_t y, uint32_t value) {
    uint32_t divisor = 1000000000u;
    uint8_t started = 0u;
    do {
        const uint32_t digit = value / divisor;
        value %= divisor;
        if (digit != 0u || started != 0u || divisor == 1u) {
            (void)sb_display_glyph(x, y, digit_glyph(digit), 0xE9F2FFu);
            x += 10u;
            started = 1u;
        }
        divisor /= 10u;
    } while (divisor != 0u);
}

static void draw_rows(void) {
    char names[512];
    const uint64_t result = sb_fs_list_root(names, sizeof(names));
    (void)sb_display_rect(32u, 72u, 376u, 360u, 0x202A38u);
    if (result == UINT64_MAX) {
        (void)sb_display_glyph(48u, 96u, FILES_ICON, 0xFF8080u);
        return;
    }
    uint32_t offset = 0u;
    uint32_t row = 0u;
    while (offset < (uint32_t)result && row < 8u) {
        uint32_t length = 0u;
        while (offset + length < (uint32_t)result && names[offset + length] != '\0') ++length;
        if (length == 0u) { ++offset; continue; }
        const uint32_t y = 92u + row * 40u;
        (void)sb_display_rect(44u, y - 4u, 352u, 34u, row & 1u ? 0x27313Eu : 0x242D39u);
        (void)sb_display_glyph(52u, y, FILE_ICON, 0xE9F2FFu);
        draw_u32(88u, y, row + 1u);
        const uint32_t name_length = length > 12u ? 12u : length;
        uint64_t marker = 0u;
        for (uint32_t i = 0u; i < name_length; ++i) marker ^= (uint64_t)(uint8_t)names[offset + i] << ((i & 7u) * 8u);
        (void)sb_display_glyph(180u, y, FILE_ICON ^ marker, 0xB8C4D4u);
        ++row;
        offset += length + 1u;
    }
    (void)sb_display_rect(32u, 448u, 376u, 40u, 0x27313Eu);
    draw_u32(48u, 460u, row);
}

uint64_t sb_app_main(void) {
    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 24u, FILES_ICON, 0xE9F2FFu);
    draw_rows();
    for (;;) {
        const uint64_t key = sb_input_key();
        if (key == 0u) { (void)sb_yield(); continue; }
        if ((key & 0x80u) != 0u) continue;
        if (key == 0x01u) return 0u;
        if (key == 0x2Bu) draw_rows();
    }
}
