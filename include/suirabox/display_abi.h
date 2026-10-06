#ifndef SUIRABOX_DISPLAY_ABI_H
#define SUIRABOX_DISPLAY_ABI_H

#define SB_DISPLAY_ABI_VERSION 1
#define SB_DISPLAY_PIXEL_RGB888 1
#define SB_DISPLAY_PRESENT_MAX_PIXELS 256
#define SB_DISPLAY_INFO_SIZE 32
#define SB_DISPLAY_PRESENT_SIZE 32
#define SB_DISPLAY_PRESENT_VERSION_OFFSET 4
#define SB_DISPLAY_PRESENT_FLAGS_OFFSET 6
#define SB_DISPLAY_PRESENT_X_OFFSET 8
#define SB_DISPLAY_PRESENT_Y_OFFSET 12
#define SB_DISPLAY_PRESENT_WIDTH_OFFSET 16
#define SB_DISPLAY_PRESENT_HEIGHT_OFFSET 20
#define SB_DISPLAY_PRESENT_PIXELS_OFFSET 24

#ifndef __ASSEMBLER__
#include <stdint.h>
typedef struct {
    uint32_t version, width, height, pixel_format;
    uint32_t max_present_pixels;
    uint32_t reserved[3];
} sb_display_info_t;
typedef struct {
    uint32_t size;
    uint16_t version, flags;
    uint32_t x, y, width, height;
    uint64_t pixels;
} sb_display_present_t;
#endif
#endif
