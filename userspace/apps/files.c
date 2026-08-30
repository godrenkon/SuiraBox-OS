#include "../syscall.h"

static const uint64_t FILES_ICON = 0x007E42427840407Eu;

uint64_t sb_app_main(void) {
    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 32u, FILES_ICON, 0xE9F2FFu);
    for (;;) {
        (void)sb_yield();
    }
}
