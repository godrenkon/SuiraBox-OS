#include "text.h"
#include "display_client.h"
#include "syscall_client.h"
#include <suirabox/syscall_abi.h>

static int present(void *context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                    const uint32_t *pixels) {
    (void)context;
    const sb_display_present_t request = {
        .size = SB_DISPLAY_PRESENT_SIZE, .version = SB_DISPLAY_ABI_VERSION,
        .x = x, .y = y, .width = width, .height = height, .pixels = (uintptr_t)pixels,
    };
    return sb_user_call3(SB_SYS_DISPLAY_PRESENT, (uintptr_t)&request, sizeof(request), 0u) == 0 ? 0 : -1;
}
int sb_text_display_setup(sb_text_renderer_t *r) {
    sb_display_info_t info;
    const int64_t status = sb_user_call3(SB_SYS_DISPLAY_INFO, (uintptr_t)&info, sizeof(info), 0u);
    if (status == SB_SYS_ERROR_NOT_FOUND) return 1; /* Headless boot remains supported. */
    if (status != 0 || info.version != SB_DISPLAY_ABI_VERSION || info.pixel_format != SB_DISPLAY_PIXEL_RGB888 ||
        info.max_present_pixels < 256u || info.reserved[0] || info.reserved[1] || info.reserved[2]) return -1;
    *r = (sb_text_renderer_t){.width = info.width, .height = info.height,
        .foreground = 0xe6eef2u, .background = 0x0c1018u, .scale = 2u, .emit = present};
    return 0;
}
static int log_message(const char *text, size_t length) {
    return sb_user_call3(SB_SYS_LOG_WRITE, (uintptr_t)text, length, 0u) == (int64_t)length ? 0 : -1;
}
int sb_boot_text(void) {
    sb_text_renderer_t renderer;
    const int result = sb_text_display_setup(&renderer);
    if (result == 1) return 0;
    if (result != 0) return -1;
    static const char title[] = "SuiraBox OS";
    static const char status[] = "Starting userspace services...";
    if (sb_text_draw(&renderer, 24, 16, title, sizeof(title) - 1u) != 0 ||
        sb_text_draw(&renderer, 24, 48, status, sizeof(status) - 1u) != 0) return -1;
    static const char message[] = "Userspace: boot text rendered\r\n";
    return log_message(message, sizeof(message) - 1u);
}

#ifdef SB_DISPLAY_PROOF
int sb_text_display_proof(void) {
    sb_text_renderer_t r;
    if (sb_text_display_setup(&r) != 0) return -1;
    /* Every cell is opaque. Different backgrounds verify pixel packing and
     * spacing as well as glyph strokes in the real ring-3 display transport. */
#define DRAW(X,Y,TEXT,FG,BG,SCALE) do { \
    r.foreground = (FG); r.background = (BG); r.scale = (SCALE); \
    static const char label[] = (TEXT); \
    if (sb_text_draw(&r, (X), (Y), label, sizeof(label) - 1u) != 0) return -1; \
} while (0)
    DRAW(24,16,"SuiraBox OS",0xe6eef2u,0x152536u,2u);
    DRAW(24,96,"Home\nFiles\nSettings",0xe6eef2u,0x18394bu,2u);
    DRAW(200,82,"Graphics and text",0xe6eef2u,0x167ca4u,2u);
    DRAW(208,288,"ASCII: AaZz 0123456789",0x152536u,0xe6eef2u,2u);
    DRAW(208,312,"UTF-8: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e / \xc3\xa9 / \xf0\x9f\x8e\xae",0x152536u,0xe6eef2u,2u);
    DRAW(208,344,"A\tB\rC\nNext line",0x152536u,0xe6eef2u,1u);
    DRAW(24,(int32_t)r.height-22,"Display + text ready",0xe6eef2u,0x152536u,1u);
    DRAW((int32_t)r.width-120,(int32_t)r.height-22,"Continue",0x0c1018u,0x259b72u,1u);
    DRAW(-3,54,"Clip",0xf2ae43u,0x0c1018u,1u);
    DRAW((int32_t)r.width-5,52,"Right",0x259b72u,0x0c1018u,2u);
    DRAW(0,(int32_t)r.height-5,"Bottom",0xf2ae43u,0x152536u,2u);
#undef DRAW
    /* Preflight must reject the whole string before painting the valid 'A'
     * prefix at (0,0). Exact screenshot verification detects any partial draw. */
    static const char bad[] = {'A', (char)0xc0, (char)0xaf};
    if (sb_text_draw(&r, 0, 0, bad, sizeof(bad)) != SB_TEXT_INVALID) return -1;
    static const char message[] = "Userspace: UTF-8 bitmap text and clipping lifecycle OK\r\n";
    return log_message(message, sizeof(message) - 1u);
}
#endif
