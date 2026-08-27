#include "setup.h"
#include <stdint.h>

static volatile uint16_t *const VGA = (volatile uint16_t *)0xB8000;
static sb_setup_config_t current_config = {
    SB_LANGUAGE_ENGLISH, 0u, 0u, SB_PERFORMANCE_BALANCED, 1u
};

static void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"((uint8_t)0));
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void cell(uint32_t x, uint32_t y, char c, uint8_t color) {
    if (x < 80u && y < 25u) VGA[y * 80u + x] = ((uint16_t)color << 8) | (uint8_t)c;
}

static void clear(uint8_t color) {
    for (uint32_t y = 0; y < 25u; ++y)
        for (uint32_t x = 0; x < 80u; ++x) cell(x, y, ' ', color);
}

static void text(uint32_t x, uint32_t y, const char *s, uint8_t color) {
    while (*s && x < 80u) cell(x++, y, *s++, color);
}

static void box(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint8_t color) {
    for (uint32_t x = x0; x <= x1; ++x) {
        cell(x, y0, '-', color);
        cell(x, y1, '-', color);
    }
    for (uint32_t y = y0; y <= y1; ++y) {
        cell(x0, y, '|', color);
        cell(x1, y, '|', color);
    }
    cell(x0, y0, '+', color); cell(x1, y0, '+', color);
    cell(x0, y1, '+', color); cell(x1, y1, '+', color);
}

static uint8_t read_key(void) {
    if ((inb(0x64u) & 1u) == 0u) return 0u;
    return inb(0x60u);
}

static void delay_poll(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) io_wait();
}

static const char *language_name(uint8_t value) {
    switch (value) {
        case SB_LANGUAGE_JAPANESE: return "Japanese";
        case SB_LANGUAGE_CHINESE: return "Chinese";
        case SB_LANGUAGE_SPANISH: return "Spanish";
        default: return "English";
    }
}

static const char *performance_name(uint8_t value) {
    switch (value) {
        case SB_PERFORMANCE_PERFORMANCE: return "Performance";
        case SB_PERFORMANCE_LOW_RESOURCE: return "Low Resource";
        case SB_PERFORMANCE_CUSTOM: return "Custom";
        default: return "Balanced";
    }
}

static void render(uint8_t selected) {
    clear(0x17u);
    text(3u, 1u, "SUIRABOX", 0x1Fu);
    text(3u, 2u, "First Setup", 0x1Fu);
    text(3u, 4u, "Choose the basics now. Advanced settings can be changed later.", 0x1Fu);
    text(3u, 6u, selected == 0u ? "> Language" : "  Language", selected == 0u ? 0x1Fu : 0x1Fu);
    text(28u, 6u, language_name(current_config.language), 0x1Fu);
    text(3u, 8u, selected == 1u ? "> Region / Time Zone" : "  Region / Time Zone", 0x1Fu);
    text(28u, 8u, "Japan / JST", 0x1Fu);
    text(3u, 10u, selected == 2u ? "> Keyboard" : "  Keyboard", 0x1Fu);
    text(28u, 10u, "Japanese", 0x1Fu);
    text(3u, 12u, selected == 3u ? "> Network" : "  Network", 0x1Fu);
    text(28u, 12u, "Configure later", 0x1Fu);
    text(3u, 14u, selected == 4u ? "> Performance" : "  Performance", 0x1Fu);
    text(28u, 14u, performance_name(current_config.performance), 0x1Fu);
    box(25u, 5u, 55u, 15u, 0x1Fu);
    text(3u, 18u, "Up/Down: select   Left/Right: change   Enter: continue", 0x1Fu);
    text(3u, 20u, "Press 1-4 for language. Esc keeps the defaults.", 0x1Fu);
    text(3u, 22u, "This early text-mode setup is intentionally tiny and fast.", 0x1Fu);
}

static void change_value(uint8_t selected, int direction) {
    if (selected == 0u) {
        int value = (int)current_config.language + direction;
        if (value < 0) value = 3;
        if (value > 3) value = 0;
        current_config.language = (uint8_t)value;
    } else if (selected == 4u) {
        int value = (int)current_config.performance + direction;
        if (value < 0) value = 3;
        if (value > 3) value = 0;
        current_config.performance = (uint8_t)value;
    }
}

int sb_setup_run(sb_setup_config_t *config) {
    uint8_t selected = 0u;
    uint8_t keys_without_input = 0u;
    if (config != 0) current_config = *config;
    render(selected);

    /* Keep CI/headless boots moving: the wizard accepts input, but defaults continue automatically. */
    for (uint32_t ticks = 0u; ticks < 1200000u; ++ticks) {
        uint8_t key = read_key();
        if (key == 0u) {
            if (keys_without_input < 255u) ++keys_without_input;
            continue;
        }
        keys_without_input = 0u;
        if (key & 0x80u) continue;
        switch (key) {
            case 0x48u: if (selected > 0u) --selected; render(selected); break;
            case 0x50u: if (selected < 4u) ++selected; render(selected); break;
            case 0x4Bu: change_value(selected, -1); render(selected); break;
            case 0x4Du: change_value(selected, 1); render(selected); break;
            case 0x1Cu: if (config != 0) *config = current_config; return 0;
            case 0x01u: if (config != 0) *config = current_config; return 0;
            case 0x02u: current_config.language = SB_LANGUAGE_JAPANESE; render(selected); break;
            case 0x03u: current_config.language = SB_LANGUAGE_ENGLISH; render(selected); break;
            case 0x04u: current_config.language = SB_LANGUAGE_CHINESE; render(selected); break;
            case 0x05u: current_config.language = SB_LANGUAGE_SPANISH; render(selected); break;
            default: break;
        }
        delay_poll(64u);
    }

    if (config != 0) *config = current_config;
    return 0;
}

const sb_setup_config_t *sb_setup_current(void) {
    return &current_config;
}
