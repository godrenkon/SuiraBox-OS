#include "../syscall.h"

static const uint64_t TERMINAL_ICON = 0x007E08080C08087Eu;
static const uint64_t G_H = 0x0042427E42424242ULL;
static const uint64_t G_T = 0x007E181818181818ULL;
static const uint64_t G_P = 0x007C42427C404040ULL;
static const uint64_t G_C = 0x003C42404040423CULL;
static const uint64_t G_Q = 0x003C42424A4C423CULL;
static const uint64_t G_0 = 0x003C42464A523C00ULL;
static const uint64_t G_1 = 0x0010181010107C00ULL;
static const uint64_t G_2 = 0x003C42021C20427EULL;
static const uint64_t G_3 = 0x007E020C02423C00ULL;
static const uint64_t G_4 = 0x000C1424447E0400ULL;
static const uint64_t G_5 = 0x007E407C02423C00ULL;
static const uint64_t G_6 = 0x001C20407C42423CULL;
static const uint64_t G_7 = 0x007E020408101000ULL;
static const uint64_t G_8 = 0x003C42423C42423CULL;
static const uint64_t G_9 = 0x003C42427E020438ULL;
static const uint64_t G_A = 0x003C42427E424242ULL;
static const uint64_t G_B = 0x007C42427C42427CULL;
static const uint64_t G_D = 0x0078424242424278ULL;
static const uint64_t G_E = 0x007E40407C40407EULL;
static const uint64_t G_F = 0x007E40407C404040ULL;

static uint64_t glyph_hex(uint8_t value) {
    switch (value & 0x0Fu) {
        case 0x0u: return G_0; case 0x1u: return G_1; case 0x2u: return G_2; case 0x3u: return G_3;
        case 0x4u: return G_4; case 0x5u: return G_5; case 0x6u: return G_6; case 0x7u: return G_7;
        case 0x8u: return G_8; case 0x9u: return G_9; case 0xAu: return G_A; case 0xBu: return G_B;
        case 0xCu: return G_C; case 0xDu: return G_D; case 0xEu: return G_E; default: return G_F;
    }
}

static void draw_hex_u64(uint32_t x, uint32_t y, uint64_t value) {
    for (int32_t i = 15; i >= 0; --i) {
        (void)sb_display_glyph(x, y, glyph_hex((uint8_t)(value >> (i * 4))), 0xBFD8FFu);
        x += 10u;
    }
}

static void draw_menu(void) {
    (void)sb_display_rect(24u, 88u, 420u, 220u, 0x202A38u);
    (void)sb_display_glyph_pair(48u, 108u, G_H, G_P, 0xE9F2FFu);
    (void)sb_display_glyph_pair(48u, 148u, G_T, G_P, 0xBFD8FFu);
    (void)sb_display_glyph_pair(48u, 188u, G_P, G_C, 0xBFD8FFu);
    (void)sb_display_glyph_pair(48u, 228u, G_C, G_Q, 0xBFD8FFu);
}

uint64_t sb_app_main(void) {
    (void)sb_display_clear(0x10151Bu);
    (void)sb_display_glyph(32u, 32u, TERMINAL_ICON, 0xE9F2FFu);
    draw_menu();

    for (;;) {
        const uint64_t key = sb_input_key();
        if (key == 0u || (key & 0x80u) != 0u) {
            (void)sb_yield();
            continue;
        }
        switch ((uint8_t)key) {
            case 0x23u:
                draw_menu();
                break;
            case 0x14u:
                (void)sb_display_rect(260u, 108u, 160u, 40u, 0x172033u);
                draw_hex_u64(268u, 124u, sb_get_ticks());
                break;
            case 0x19u:
                (void)sb_display_rect(260u, 148u, 160u, 40u, 0x172033u);
                draw_hex_u64(268u, 164u, sb_process_id());
                break;
            case 0x2Eu:
                (void)sb_display_clear(0x10151Bu);
                (void)sb_display_glyph(32u, 32u, TERMINAL_ICON, 0xE9F2FFu);
                draw_menu();
                break;
            case 0x10u:
            case 0x01u:
                return 0u;
            default:
                break;
        }
    }
}
