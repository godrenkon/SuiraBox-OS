#include "../syscall.h"

#define FILES_MAX_NAMES 12u
#define FILES_MAX_PREVIEW 128u

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
static const uint64_t GA = 0x003C42427E424242u;
static const uint64_t GB = 0x007C42427C42427Cu;
static const uint64_t GC = 0x003C42404040423Cu;
static const uint64_t GD = 0x0078424242424278u;
static const uint64_t GE = 0x007E40407C40407Eu;
static const uint64_t GF = 0x007E40407C404040u;
static const uint64_t GH = 0x0042427E42424242u;
static const uint64_t GI = 0x007E18181818187Eu;
static const uint64_t GJ = 0x001E080808484830u;
static const uint64_t GK = 0x0042444870484400u;
static const uint64_t GL = 0x004040404040407Eu;
static const uint64_t GM = 0x0042665A42424242u;
static const uint64_t GN = 0x004266525A424242u;
static const uint64_t GO = 0x003C42424242423Cu;
static const uint64_t GP = 0x007C42427C404040u;
static const uint64_t GQ = 0x003C42424A4C423Cu;
static const uint64_t GR = 0x007C42427C484442u;
static const uint64_t GS = 0x003C40403C02023Cu;
static const uint64_t GT = 0x007E181818181818u;
static const uint64_t GU = 0x004242424242423Cu;
static const uint64_t GV = 0x0042242424181800u;
static const uint64_t GW = 0x004242425A5A6666u;
static const uint64_t GX = 0x0042241818244242u;
static const uint64_t GY = 0x0042241818181800u;
static const uint64_t GZ = 0x007E02040810207Eu;

static uint64_t glyph_for(char c) {
    static const uint64_t digits[10] = {G_0,G_1,G_2,G_3,G_4,G_5,G_6,G_7,G_8,G_9};
    switch (c) {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return digits[(uint32_t)c - '0'];
        case 'A': return GA; case 'B': return GB; case 'C': return GC; case 'D': return GD; case 'E': return GE; case 'F': return GF;
        case 'G': return GC; case 'H': return GH; case 'I': return GI; case 'J': return GJ; case 'K': return GK; case 'L': return GL; case 'M': return GM; case 'N': return GN;
        case 'O': return GO; case 'P': return GP; case 'Q': return GQ; case 'R': return GR; case 'S': return GS; case 'T': return GT; case 'U': return GU;
        case 'V': return GV; case 'W': return GW; case 'X': return GX; case 'Y': return GY; case 'Z': return GZ; default: return GC;
    }
}

static void draw_char(uint32_t x, uint32_t y, char c, uint32_t rgb) {
    if (c != ' ') (void)sb_display_glyph(x, y, glyph_for(c), rgb);
}

static void draw_text(uint32_t x, uint32_t y, const char *text, uint32_t rgb) {
    if (text == 0) return;
    while (*text != '\0') { draw_char(x, y, *text++, rgb); x += 10u; }
}

static void draw_u32(uint32_t x, uint32_t y, uint32_t value, uint32_t rgb) {
    uint32_t divisor = 1000000000u;
    uint8_t started = 0u;
    do {
        const uint32_t digit = value / divisor;
        value %= divisor;
        if (digit != 0u || started != 0u || divisor == 1u) { draw_char(x, y, (char)('0' + digit), rgb); x += 10u; started = 1u; }
        divisor /= 10u;
    } while (divisor != 0u);
}

static int load_names(char names[][13], uint32_t *count) {
    char raw[512];
    const uint64_t result = sb_fs_list_root(raw, sizeof(raw));
    uint32_t offset = 0u;
    uint32_t written = 0u;
    if (count == 0 || result == UINT64_MAX) return -1;
    *count = 0u;
    while (offset < (uint32_t)result && *count < FILES_MAX_NAMES) {
        uint32_t length = 0u;
        while (offset + length < (uint32_t)result && raw[offset + length] != '\0') ++length;
        if (length != 0u) {
            const uint32_t copy_len = length > FILES_MAX_NAMES ? FILES_MAX_NAMES : length;
            for (uint32_t i = 0u; i < copy_len; ++i) names[*count][i] = raw[offset + i];
            names[*count][copy_len] = '\0';
            ++*count;
            written += copy_len + 1u;
        }
        offset += length + 1u;
    }
    (void)written;
    return 0;
}

static void draw_preview(const char *name, uint32_t selected) {
    char path[32];
    char data[FILES_MAX_PREVIEW];
    uint32_t name_len = 0u;
    while (name != 0 && name[name_len] != '\0' && name_len < 12u) ++name_len;
    if (name == 0 || name_len == 0u || name_len + 1u >= sizeof(path)) return;
    path[0] = '/';
    for (uint32_t i = 0u; i < name_len; ++i) path[i + 1u] = name[i];
    path[name_len + 1u] = '\0';

    (void)sb_display_rect(432u, 72u, 328u, 360u, 0x202A38u);
    draw_text(448u, 92u, "PREVIEW", 0x7FA8D8u);
    draw_text(448u, 120u, path + 1u, 0xE9F2FFu);
    const uint64_t fd = sb_fs_open(path, name_len + 1u, SB_FS_OPEN_READ, 0u);
    if (fd == UINT64_MAX) { draw_text(448u, 152u, "OPEN ERROR", 0xFF8080u); return; }

    uint32_t total = 0u;
    for (;;) {
        const uint64_t result = sb_fs_read(fd, data + total, FILES_MAX_PREVIEW - total - 1u);
        if (result == UINT64_MAX) { draw_text(448u, 152u, "READ ERROR", 0xFF8080u); (void)sb_fs_close(fd); return; }
        if (result == 0u) break;
        total += (uint32_t)result;
        if (total >= FILES_MAX_PREVIEW - 1u) break;
    }
    (void)sb_fs_close(fd);
    data[total] = '\0';
    draw_text(448u, 148u, "SIZE", 0x7FA8D8u);
    draw_u32(498u, 148u, total, 0xE9F2FFu);
    draw_text(448u, 184u, "CONTENT", 0x7FA8D8u);
    uint32_t x = 448u;
    uint32_t y = 212u;
    for (uint32_t i = 0u; i < total; ++i) {
        char c = data[i];
        if (c == '\n' || x >= 742u) { x = 448u; y += 24u; if (c == '\n') continue; }
        if (y >= 408u) break;
        if (c < ' ' || c > '~') c = '?';
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        draw_char(x, y, c, 0xBFD8FFu);
        x += 10u;
    }
    (void)selected;
}

static uint32_t draw_rows(char names[][13], uint32_t count, uint32_t selected) {
    (void)sb_display_rect(32u, 72u, 376u, 360u, 0x202A38u);
    for (uint32_t row = 0u; row < count && row < FILES_MAX_NAMES; ++row) {
        const uint32_t y = 92u + row * 40u;
        const uint32_t bg = row == selected ? 0x36536Eu : (row & 1u ? 0x27313Eu : 0x242D39u);
        (void)sb_display_rect(44u, y - 4u, 352u, 34u, bg);
        (void)sb_display_glyph(52u, y, FILE_ICON, row == selected ? 0xFFFFFFu : 0xE9F2FFu);
        draw_u32(88u, y, row + 1u, 0xE9F2FFu);
        uint64_t marker = 0u;
        for (uint32_t i = 0u; i < 12u && names[row][i] != '\0'; ++i) marker ^= (uint64_t)(uint8_t)names[row][i] << ((i & 7u) * 8u);
        (void)sb_display_glyph(180u, y, FILE_ICON ^ marker, 0xB8C4D4u);
        const uint32_t name_length = 13u;
        (void)name_length;
    }
    (void)sb_display_rect(32u, 448u, 728u, 40u, 0x27313Eu);
    draw_text(48u, 460u, "ENTER OPEN", 0xBFD8FFu);
    return count;
}

static void redraw(char names[][13], uint32_t count, uint32_t selected) {
    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 24u, FILES_ICON, 0xE9F2FFu);
    draw_text(82u, 28u, "FILES", 0xE9F2FFu);
    draw_rows(names, count, selected);
    if (count != 0u) draw_preview(names[selected], selected);
}

uint64_t sb_app_main(void) {
    char names[FILES_MAX_NAMES][13] = {{0}};
    uint32_t count = 0u;
    uint32_t selected = 0u;
    if (load_names(names, &count) != 0) count = 0u;
    redraw(names, count, selected);
    for (;;) {
        const uint64_t key = sb_input_key();
        if (key == 0u) { (void)sb_yield(); continue; }
        if ((key & 0x80u) != 0u) continue;
        if (key == 0x01u) return 0u;
        if (key == 0x48u) {
            if (count != 0u && selected > 0u) { --selected; redraw(names, count, selected); }
        } else if (key == 0x50u) {
            if (count != 0u && selected + 1u < count) { ++selected; redraw(names, count, selected); }
        } else if (key == 0x1Cu) {
            if (count != 0u) draw_preview(names[selected], selected);
        } else if (key == 0x2Bu) {
            if (load_names(names, &count) != 0) count = 0u;
            if (count == 0u) selected = 0u; else if (selected >= count) selected = count - 1u;
            redraw(names, count, selected);
        }
    }
}
