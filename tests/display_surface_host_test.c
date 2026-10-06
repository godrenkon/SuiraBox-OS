#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "display_surface.h"

static uint8_t memory[64], before[64];
static int check(int ok, const char *why) {
    if (ok) return 0;
    fprintf(stderr, "Display surface failed: %s\n", why); return 1;
}
static sb_display_surface_t start(void) {
    memset(memory, 0xA5, sizeof(memory));
    return (sb_display_surface_t){.pixels = memory + 8u, .byte_length = 24u,
        .width = 2u, .height = 2u, .pitch = 12u, .bits_per_pixel = 32u,
        .red_position = 16u, .red_size = 8u, .green_position = 8u, .green_size = 8u,
        .blue_position = 0u, .blue_size = 8u};
}
static int guards(void) {
    for (uint32_t i = 0u; i < sizeof(memory); ++i)
        if ((i < 8u || i >= 32u || (i >= 16u && i < 20u) || (i >= 28u && i < 32u)) && memory[i] != 0xA5u) return 0;
    return 1;
}
int main(void) {
    const uint32_t rgb[] = {0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF123456u};
    sb_display_surface_t surface = start();
    if (check(sb_display_surface_valid(&surface) && sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, rgb, 4u) == 0,
        "packed RGB source to padded 32-bit surface")) return 1;
    const uint8_t expected[] = {0,0,255,0, 0,255,0,0, 0xA5,0xA5,0xA5,0xA5,
                                255,0,0,0, 0x56,0x34,0x12,0, 0xA5,0xA5,0xA5,0xA5};
    if (check(!memcmp(memory + 8u, expected, sizeof(expected)) && guards(), "exact colors; padding, high byte and guards")) return 1;
    memcpy(before, memory, sizeof(memory));
    if (check(sb_display_surface_present(&surface, UINT32_MAX, 0u, 1u, 1u, rgb, 1u) != 0 &&
        sb_display_surface_present(&surface, 1u, 1u, 2u, 1u, rgb, 2u) != 0 &&
        sb_display_surface_present(&surface, 0u, 0u, 0u, 1u, rgb, 0u) != 0 &&
        sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, rgb, 3u) != 0 &&
        sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, rgb, 5u) != 0 &&
        sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, 0, 4u) != 0 &&
        sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, (const uint32_t *)(uintptr_t)(UINTPTR_MAX - 4u), 4u) != 0 &&
        !memcmp(memory, before, sizeof(memory)), "every rejected present has no partial write")) return 1;
    if (check(sb_display_surface_fill(&surface, 1u, 1u, UINT32_MAX, UINT32_MAX, 255u, 255u, 255u) == 0 &&
        memory[24] == 255u && memory[25] == 255u && memory[26] == 255u && memory[27] == 0u && guards(),
        "fill clipping handles overflowing input extents")) return 1;
    surface = start(); surface.bits_per_pixel = 24u; surface.pitch = 8u; surface.byte_length = 16u;
    surface.red_position = 0u; surface.blue_position = 16u;
    if (check(sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, rgb, 4u) == 0 &&
        !memcmp(memory + 8u, "\xFF\x00\x00\x00\xFF\x00", 6u) && memory[14] == 0xA5 && memory[15] == 0xA5 &&
        !memcmp(memory + 16u, "\x00\x00\xFF\x12\x34\x56", 6u) && memory[22] == 0xA5 && memory[23] == 0xA5,
        "24-bit reversed channel positions, row padding retained")) return 1;
    surface = start(); surface.bits_per_pixel = 16u; surface.pitch = 6u; surface.byte_length = 12u;
    surface.red_position = 11u; surface.red_size = 5u; surface.green_position = 5u; surface.green_size = 6u;
    surface.blue_size = 5u;
    const uint32_t rgb565[] = {0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0x00808080u};
    if (check(sb_display_surface_present(&surface, 0u, 0u, 2u, 2u, rgb565, 4u) == 0 &&
        !memcmp(memory + 8u, "\x00\xF8\xE0\x07", 4u) && !memcmp(memory + 14u, "\x1F\x00\x10\x84", 4u) &&
        memory[12] == 0xA5 && memory[13] == 0xA5 && memory[18] == 0xA5 && memory[19] == 0xA5,
        "RGB565 scaling and padding")) return 1;
    for (int fault = 0; fault < 11; ++fault) {
        surface = start();
        if (fault == 0) surface.pitch = 7u;
        if (fault == 1) surface.byte_length = 23u;
        if (fault == 2) surface.red_size = 0u;
        if (fault == 3) surface.green_position = 16u;
        if (fault == 4) surface.red_position = 31u;
        if (fault == 5) surface.bits_per_pixel = 8u;
        if (fault == 6) surface.width = UINT32_MAX;
        if (fault == 7) surface.pixels = 0;
        if (fault == 8) surface.height = UINT32_MAX;
        if (fault == 9) surface.red_size = 32u;
        if (fault == 10) surface.pixels = (volatile uint8_t *)(uintptr_t)(UINTPTR_MAX - 8u);
        memcpy(before, memory, sizeof(memory));
        if (check(!sb_display_surface_valid(&surface) &&
            sb_display_surface_present(&surface, 0u, 0u, 1u, 1u, rgb, 1u) != 0 &&
            sb_display_surface_fill(&surface, 0u, 0u, 1u, 1u, 255u, 0u, 0u) != 0 &&
            !memcmp(memory, before, sizeof(memory)), "malformed surfaces never touch memory")) return 1;
    }
    puts("Display surface host test OK"); return 0;
}
