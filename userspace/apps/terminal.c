#include "../syscall.h"

static const uint64_t TERMINAL_ICON = 0x007E08080C08087Eu;

uint64_t sb_app_main(void) {
    (void)sb_display_clear(0x10151Bu);
    (void)sb_display_glyph(32u, 32u, TERMINAL_ICON, 0xE9F2FFu);
    for (;;) {
        (void)sb_yield();
    }
}
