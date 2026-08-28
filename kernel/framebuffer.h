#ifndef SUIRABOX_FRAMEBUFFER_H
#define SUIRABOX_FRAMEBUFFER_H

#include <stdint.h>

#define SB_FRAMEBUFFER_VIRTUAL_BASE 0x0000005000000000ull

typedef struct {
    uint64_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bits_per_pixel;
    uint8_t type;
    uint8_t red_position;
    uint8_t red_mask_size;
    uint8_t green_position;
    uint8_t green_mask_size;
    uint8_t blue_position;
    uint8_t blue_mask_size;
    uint64_t mapped_address;
    uint64_t mapped_size;
} sb_framebuffer_info_t;

int sb_framebuffer_init(uint64_t multiboot_info_address);
int sb_framebuffer_available(void);
const sb_framebuffer_info_t *sb_framebuffer_info(void);
int sb_framebuffer_map(void);
int sb_framebuffer_clear(uint8_t red, uint8_t green, uint8_t blue);
int sb_framebuffer_draw_pixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue);
int sb_framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                             uint8_t red, uint8_t green, uint8_t blue);
int sb_framebuffer_draw_glyph8(uint32_t x, uint32_t y, uint64_t bitmap,
                               uint8_t red, uint8_t green, uint8_t blue);

#endif
