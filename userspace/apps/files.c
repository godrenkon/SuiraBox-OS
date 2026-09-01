#include "../syscall.h"

#define FILES_MAX_ENTRIES 12u
#define FILES_MAX_PREVIEW 128u
#define FILES_MAX_PATH 64u

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

static int record_name(const sb_fs_dir_record_t *entry, char out[13]) {
    if (entry == 0 || out == 0 || entry->name_length == 0u || entry->name_length >= 13u) return -1;
    for (uint32_t i = 0u; i < entry->name_length; ++i) out[i] = entry->name[i];
    out[entry->name_length] = '\0';
    return 0;
}

static int name_length(const char *name, uint32_t *length) {
    uint32_t value = 0u;
    if (name == 0 || length == 0) return -1;
    while (value < 12u && name[value] != '\0') ++value;
    if (name[value] != '\0') return -1;
    *length = value;
    return 0;
}

static int join_path(const char *base, const char *name, char out[FILES_MAX_PATH]) {
    uint32_t base_len = 0u;
    uint32_t name_len = 0u;
    if (base == 0 || name == 0 || out == 0 || name_length(name, &name_len) != 0) return -1;
    while (base[base_len] != '\0') {
        if (base_len + 1u >= FILES_MAX_PATH) return -1;
        ++base_len;
    }
    if (base_len == 1u && base[0] == '/') {
        if (1u + name_len + 1u > FILES_MAX_PATH) return -1;
        out[0] = '/';
        for (uint32_t i = 0u; i < name_len; ++i) out[i + 1u] = name[i];
        out[name_len + 1u] = '\0';
        return 0;
    }
    if (base_len + 1u + name_len + 1u > FILES_MAX_PATH) return -1;
    for (uint32_t i = 0u; i < base_len; ++i) out[i] = base[i];
    out[base_len] = '/';
    for (uint32_t i = 0u; i < name_len; ++i) out[base_len + 1u + i] = name[i];
    out[base_len + 1u + name_len] = '\0';
    return 0;
}

static int parent_path(const char *current, char out[FILES_MAX_PATH]) {
    uint32_t length = 0u;
    uint32_t slash = 0u;
    if (current == 0 || out == 0 || current[0] != '/') return -1;
    if (current[1] == '\0') { out[0] = '/'; out[1] = '\0'; return 0; }
    while (current[length] != '\0') {
        if (length + 1u >= FILES_MAX_PATH) return -1;
        if (current[length] == '/') slash = length;
        ++length;
    }
    if (slash == 0u) { out[0] = '/'; out[1] = '\0'; return 0; }
    for (uint32_t i = 0u; i < slash; ++i) out[i] = current[i];
    out[slash] = '\0';
    return 0;
}

static int load_entries(const char *path, sb_fs_dir_record_t entries[], uint32_t *count) {
    const uint32_t capacity = FILES_MAX_ENTRIES * SB_FS_DIR_RECORD_SIZE;
    uint32_t path_len = 0u;
    uint64_t result;
    if (path == 0 || entries == 0 || count == 0) return -1;
    while (path[path_len] != '\0') {
        if (path_len + 1u >= FILES_MAX_PATH) return -1;
        ++path_len;
    }
    result = sb_fs_list(path, path_len, entries, capacity);
    if (result == UINT64_MAX || result > capacity || result % SB_FS_DIR_RECORD_SIZE != 0u) return -1;
    *count = (uint32_t)(result / SB_FS_DIR_RECORD_SIZE);
    for (uint32_t i = 0u; i < *count; ++i) {
        if (entries[i].name_length == 0u || entries[i].name_length > sizeof(entries[i].name)) return -1;
    }
    return 0;
}

static void draw_preview(const char *base_path, const sb_fs_dir_record_t *entry) {
    char path[FILES_MAX_PATH];
    char name[13];
    char data[FILES_MAX_PREVIEW];
    if (base_path == 0 || entry == 0 || record_name(entry, name) != 0) return;
    (void)sb_display_rect(432u, 72u, 328u, 360u, 0x202A38u);
    draw_text(448u, 92u, "PREVIEW", 0x7FA8D8u);
    draw_text(448u, 120u, name, 0xE9F2FFu);
    if (entry->type == SB_FS_DIR_TYPE_DIRECTORY) {
        draw_text(448u, 160u, "DIRECTORY", 0xBFD8FFu);
        draw_text(448u, 192u, "ENTER TO OPEN", 0xBFD8FFu);
        return;
    }
    if (join_path(base_path, name, path) != 0) return;
    const uint32_t path_len = name_length(path, &(uint32_t){0}) == 0 ? (uint32_t)__builtin_strlen(path) : 0u;
    const uint64_t fd = sb_fs_open(path, path_len, SB_FS_OPEN_READ, 0u);
    if (fd == UINT64_MAX) { draw_text(448u, 152u, "OPEN ERROR", 0xFF8080u); return; }

    uint32_t total = 0u;
    for (;;) {
        const uint32_t available = FILES_MAX_PREVIEW - total - 1u;
        const uint64_t result = sb_fs_read(fd, data + total, available);
        if (result == UINT64_MAX) { draw_text(448u, 152u, "READ ERROR", 0xFF8080u); (void)sb_fs_close(fd); return; }
        if (result == 0u || available == 0u) break;
        total += (uint32_t)result;
        if ((uint32_t)result < available) break;
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
}

static void draw_rows(const sb_fs_dir_record_t entries[], uint32_t count, uint32_t selected) {
    (void)sb_display_rect(32u, 72u, 376u, 360u, 0x202A38u);
    for (uint32_t row = 0u; row < count && row < FILES_MAX_ENTRIES; ++row) {
        char name[13];
        if (record_name(&entries[row], name) != 0) continue;
        const uint32_t y = 92u + row * 40u;
        const uint32_t bg = row == selected ? 0x36536Eu : (row & 1u ? 0x27313Eu : 0x242D39u);
        const uint64_t icon = entries[row].type == SB_FS_DIR_TYPE_DIRECTORY ? FILES_ICON : FILE_ICON;
        (void)sb_display_rect(44u, y - 4u, 352u, 34u, bg);
        (void)sb_display_glyph(52u, y, icon, row == selected ? 0xFFFFFFu : 0xE9F2FFu);
        draw_u32(88u, y, row + 1u, 0xE9F2FFu);
        draw_text(180u, y, name, row == selected ? 0xFFFFFFu : 0xB8C4D4u);
        if (entries[row].type == SB_FS_DIR_TYPE_DIRECTORY) draw_text(330u, y, "DIR", 0x7FA8D8u);
    }
    (void)sb_display_rect(32u, 448u, 728u, 40u, 0x27313Eu);
    draw_text(48u, 460u, "ENTER OPEN  BACK PARENT  F5 REFRESH", 0xBFD8FFu);
}

static void redraw(const char *path, const sb_fs_dir_record_t entries[], uint32_t count, uint32_t selected) {
    (void)sb_display_clear(0x18202Au);
    (void)sb_display_glyph(32u, 24u, FILES_ICON, 0xE9F2FFu);
    draw_text(82u, 28u, "FILES", 0xE9F2FFu);
    draw_text(160u, 28u, path, 0x7FA8D8u);
    draw_rows(entries, count, selected);
    if (count != 0u) draw_preview(path, &entries[selected]);
}

uint64_t sb_app_main(void) {
    char current_path[FILES_MAX_PATH] = "/";
    sb_fs_dir_record_t entries[FILES_MAX_ENTRIES] = {{0}};
    uint32_t count = 0u;
    uint32_t selected = 0u;

    if (load_entries(current_path, entries, &count) != 0) count = 0u;
    redraw(current_path, entries, count, selected);
    for (;;) {
        const uint64_t key = sb_input_key();
        if (key == 0u) { (void)sb_yield(); continue; }
        if ((key & 0x80u) != 0u) continue;
        if (key == 0x01u) return 0u;
        if (key == 0x48u) {
            if (count != 0u && selected > 0u) { --selected; redraw(current_path, entries, count, selected); }
        } else if (key == 0x50u) {
            if (count != 0u && selected + 1u < count) { ++selected; redraw(current_path, entries, count, selected); }
        } else if (key == 0x1Cu) {
            if (count == 0u) continue;
            if (entries[selected].type == SB_FS_DIR_TYPE_DIRECTORY) {
                char name[13];
                char next_path[FILES_MAX_PATH];
                if (record_name(&entries[selected], name) != 0 || join_path(current_path, name, next_path) != 0) continue;
                for (uint32_t i = 0u; i < sizeof(current_path); ++i) current_path[i] = next_path[i];
                if (load_entries(current_path, entries, &count) != 0) { count = 0u; }
                selected = 0u;
                redraw(current_path, entries, count, selected);
            } else {
                draw_preview(current_path, &entries[selected]);
            }
        } else if (key == 0x0Eu) {
            char next_path[FILES_MAX_PATH];
            if (current_path[1] == '\0' || parent_path(current_path, next_path) != 0) continue;
            for (uint32_t i = 0u; i < sizeof(current_path); ++i) current_path[i] = next_path[i];
            if (load_entries(current_path, entries, &count) != 0) count = 0u;
            selected = 0u;
            redraw(current_path, entries, count, selected);
        } else if (key == 0x2Bu || key == 0x3Fu) {
            if (load_entries(current_path, entries, &count) != 0) count = 0u;
            if (count == 0u) selected = 0u; else if (selected >= count) selected = count - 1u;
            redraw(current_path, entries, count, selected);
        }
    }
}