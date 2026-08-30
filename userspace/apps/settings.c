#include "../syscall.h"

static const uint64_t SETTINGS_ICON = 0x003C427E625A423Cu;

uint64_t sb_app_main(void) {
    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 32u, SETTINGS_ICON, 0xE9F2FFu);
    for (;;) {
        (void)sb_yield();
    }
}
