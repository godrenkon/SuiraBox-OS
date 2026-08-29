#include "framebuffer.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include <stdint.h>

#define SB_MB2_MAX_INFO_SIZE (64u * 1024u)
#define SB_MB2_MAX_TAGS 128u
#define SB_IDENTITY_MAP_LIMIT (1ull << 30)
#define SB_MB2_TAG_END 0u
#define SB_MB2_TAG_FRAMEBUFFER 8u
#define SB_FB_DIRECT 1u
#define SB_FB_MAX_BPP 32u
#define SB_PAGE_MASK (~(uint64_t)(SB_PAGE_SIZE - 1u))

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t size;
} sb_mb2_tag_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t size;
    uint64_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} sb_mb2_fb_tag_prefix_t;

typedef struct __attribute__((packed)) {
    uint8_t red_position;
    uint8_t red_mask_size;
    uint8_t green_position;
    uint8_t green_mask_size;
    uint8_t blue_position;
    uint8_t blue_mask_size;
} sb_mb2_fb_direct_info_t;

static sb_framebuffer_info_t current;
static int available;

static uint64_t align8(uint64_t value) {
    if (value > UINT64_MAX - 7u) return UINT64_MAX;
    return (value + 7u) & ~7ull;
}

static uint32_t component_mask(uint8_t bits) {
    if (bits == 0u) return 0u;
    if (bits >= 32u) return UINT32_MAX;
    return (1u << bits) - 1u;
}

static uint32_t scale_component(uint8_t value, uint8_t bits) {
    const uint32_t max_value = component_mask(bits);
    if (max_value == 0u) return 0u;
    return ((uint32_t)value * max_value + 127u) / 255u;
}

static int framebuffer_geometry_ok(void) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    const uint64_t row_bytes = (uint64_t)current.width * bytes_per_pixel;
    const uint64_t span = (uint64_t)current.pitch * current.height;
    if (current.type != SB_FB_DIRECT || bytes_per_pixel == 0u || bytes_per_pixel > 4u ||
        current.bits_per_pixel > SB_FB_MAX_BPP || current.address == 0u || current.pitch == 0u ||
        row_bytes > current.pitch || span == 0u || span > UINT64_MAX - current.address)
        return 0;
    if (current.red_position >= 32u || current.green_position >= 32u || current.blue_position >= 32u ||
        current.red_mask_size > 32u || current.green_mask_size > 32u || current.blue_mask_size > 32u ||
        (uint16_t)current.red_position + current.red_mask_size > current.bits_per_pixel ||
        (uint16_t)current.green_position + current.green_mask_size > current.bits_per_pixel ||
        (uint16_t)current.blue_position + current.blue_mask_size > current.bits_per_pixel)
        return 0;
    return 1;
}

static int framebuffer_target(uint64_t *target_address) {
    if (target_address == 0 || !available || !framebuffer_geometry_ok()) return -1;
    if (current.mapped_address != 0u) {
        if (current.mapped_size == 0u) return -2;
        *target_address = current.mapped_address;
        return 0;
    }

    {
        const uint64_t span = (uint64_t)current.pitch * current.height;
        const uint64_t end = current.address + span;
        if (current.address >= SB_IDENTITY_MAP_LIMIT || end > SB_IDENTITY_MAP_LIMIT)
            return -3;
    }
    *target_address = current.address;
    return 0;
}

static uint32_t pack_pixel(uint8_t red, uint8_t green, uint8_t blue) {
    if (current.bits_per_pixel == 32u &&
        current.red_position == 16u && current.red_mask_size == 8u &&
        current.green_position == 8u && current.green_mask_size == 8u &&
        current.blue_position == 0u && current.blue_mask_size == 8u)
        return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
    {
        uint32_t pixel = scale_component(red, current.red_mask_size) << current.red_position;
        pixel |= scale_component(green, current.green_mask_size) << current.green_position;
        pixel |= scale_component(blue, current.blue_mask_size) << current.blue_position;
        return pixel;
    }
}

static void store_pixel(volatile uint8_t *dst, uint64_t bytes_per_pixel, uint32_t pixel) {
    for (uint64_t byte = 0u; byte < bytes_per_pixel; ++byte)
        dst[byte] = (uint8_t)(pixel >> (byte * 8u));
}

static int draw_glyph8_packed(uint64_t target_address, uint64_t bytes_per_pixel,
                              uint32_t x, uint32_t y, uint64_t bitmap,
                              uint32_t pixel) {
    for (uint32_t row_index = 0u; row_index < 8u; ++row_index) {
        const uint8_t row_bits = (uint8_t)(bitmap >> (row_index * 8u));
        volatile uint8_t *row = (volatile uint8_t *)(uintptr_t)(
            target_address + (uint64_t)(y + row_index) * current.pitch + (uint64_t)x * bytes_per_pixel);
        if (bytes_per_pixel == 4u) {
            volatile uint32_t *fast = (volatile uint32_t *)(void *)row;
            for (uint32_t column = 0u; column < 8u; ++column)
                if ((row_bits & (uint8_t)(1u << (7u - column))) != 0u) fast[column] = pixel;
        } else {
            for (uint32_t column = 0u; column < 8u; ++column) {
                if ((row_bits & (uint8_t)(1u << (7u - column))) != 0u)
                    store_pixel(row + (uint64_t)column * bytes_per_pixel, bytes_per_pixel, pixel);
            }
        }
    }
    return 0;
}

int sb_framebuffer_init(uint64_t multiboot_info_address) {
    available = 0;
    current = (sb_framebuffer_info_t){0};

    if (multiboot_info_address == 0u || multiboot_info_address >= 0x40000000ull)
        return 0;

    {
        const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_address;
        if (total_size < 16u || total_size > SB_MB2_MAX_INFO_SIZE ||
            (total_size & 7u) != 0u || total_size > UINT64_MAX - multiboot_info_address)
            return 0;

        uint32_t offset = 8u;
        uint32_t tags_seen = 0u;
        while (offset <= total_size - 8u && tags_seen++ < SB_MB2_MAX_TAGS) {
            const sb_mb2_tag_t *tag = (const sb_mb2_tag_t *)(uintptr_t)(multiboot_info_address + offset);
            if (tag->size < 8u || tag->size > total_size - offset) return 0;
            if (tag->type == SB_MB2_TAG_END) break;

            if (tag->type == SB_MB2_TAG_FRAMEBUFFER &&
                tag->size >= sizeof(sb_mb2_fb_tag_prefix_t)) {
                const sb_mb2_fb_tag_prefix_t *fb = (const sb_mb2_fb_tag_prefix_t *)tag;
                const uint64_t bytes_per_pixel = ((uint64_t)fb->bpp + 7u) / 8u;
                const uint64_t row_bytes = (uint64_t)fb->width * bytes_per_pixel;
                const uint64_t total_bytes = (uint64_t)fb->pitch * fb->height;
                if (fb->address != 0u && fb->pitch != 0u && fb->width != 0u &&
                    fb->height != 0u && fb->bpp != 0u && fb->bpp <= SB_FB_MAX_BPP &&
                    bytes_per_pixel <= 4u && row_bytes <= fb->pitch && total_bytes <= UINT64_MAX - fb->address) {
                    current.address = fb->address;
                    current.pitch = fb->pitch;
                    current.width = fb->width;
                    current.height = fb->height;
                    current.bits_per_pixel = fb->bpp;
                    current.type = fb->framebuffer_type;
                    if (fb->framebuffer_type == SB_FB_DIRECT &&
                        tag->size >= sizeof(sb_mb2_fb_tag_prefix_t) + sizeof(sb_mb2_fb_direct_info_t)) {
                        const sb_mb2_fb_direct_info_t *direct =
                            (const sb_mb2_fb_direct_info_t *)((const uint8_t *)tag + sizeof(sb_mb2_fb_tag_prefix_t));
                        current.red_position = direct->red_position;
                        current.red_mask_size = direct->red_mask_size;
                        current.green_position = direct->green_position;
                        current.green_mask_size = direct->green_mask_size;
                        current.blue_position = direct->blue_position;
                        current.blue_mask_size = direct->blue_mask_size;
                        available = framebuffer_geometry_ok();
                    }
                }
            }

            {
                const uint64_t next = align8(tag->size);
                if (next > UINT32_MAX || next > (uint64_t)(total_size - offset)) return 0;
                offset += (uint32_t)next;
            }
        }
    }

    return available;
}

int sb_framebuffer_available(void) { return available; }

const sb_framebuffer_info_t *sb_framebuffer_info(void) {
    return available ? &current : (const sb_framebuffer_info_t *)0;
}

int sb_framebuffer_map(void) {
    uint64_t physical_start, physical_end, page_count, virtual_start;

    if (!available || !framebuffer_geometry_ok() || current.mapped_address != 0u)
        return available && current.mapped_address != 0u;

    physical_start = current.address & SB_PAGE_MASK;
    physical_end = current.address + (uint64_t)current.pitch * current.height;
    if (physical_end > UINT64_MAX - (SB_PAGE_SIZE - 1u)) return 0;
    physical_end = (physical_end + SB_PAGE_SIZE - 1u) & SB_PAGE_MASK;
    if (physical_end <= physical_start) return 0;

    page_count = (physical_end - physical_start) / SB_PAGE_SIZE;
    if (page_count > (UINT64_MAX - SB_FRAMEBUFFER_VIRTUAL_BASE) / SB_PAGE_SIZE) return 0;
    virtual_start = SB_FRAMEBUFFER_VIRTUAL_BASE;

    for (uint64_t i = 0u; i < page_count; ++i) {
        if (vmm_map_page(virtual_start + i * SB_PAGE_SIZE,
                         physical_start + i * SB_PAGE_SIZE,
                         SB_VMM_WRITABLE) != 0) {
            for (uint64_t rollback = 0u; rollback < i; ++rollback)
                (void)vmm_unmap_page(virtual_start + rollback * SB_PAGE_SIZE, 0);
            return 0;
        }
    }

    current.mapped_address = virtual_start + (current.address - physical_start);
    current.mapped_size = physical_end - physical_start;
    return 1;
}

int sb_framebuffer_clear(uint8_t red, uint8_t green, uint8_t blue) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    uint64_t target_address;
    const uint32_t pixel = pack_pixel(red, green, blue);

    if (framebuffer_target(&target_address) != 0) return -1;

    for (uint32_t y = 0u; y < current.height; ++y) {
        volatile uint8_t *row = (volatile uint8_t *)(uintptr_t)(target_address + (uint64_t)y * current.pitch);
        if (bytes_per_pixel == 4u) {
            volatile uint32_t *fast = (volatile uint32_t *)(void *)row;
            for (uint32_t x = 0u; x < current.width; ++x) fast[x] = pixel;
        } else {
            for (uint32_t x = 0u; x < current.width; ++x)
                store_pixel(row + (uint64_t)x * bytes_per_pixel, bytes_per_pixel, pixel);
        }
    }

    return 0;
}

int sb_framebuffer_draw_pixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    uint64_t target_address;

    if (x >= current.width || y >= current.height) return -1;
    if (framebuffer_target(&target_address) != 0) return -2;
    if ((uint64_t)y * current.pitch > UINT64_MAX - (uint64_t)x * bytes_per_pixel) return -3;

    if (bytes_per_pixel == 4u) {
        volatile uint32_t *pixel = (volatile uint32_t *)(uintptr_t)(target_address +
            (uint64_t)y * current.pitch + (uint64_t)x * 4u);
        *pixel = pack_pixel(red, green, blue);
    } else {
        store_pixel((volatile uint8_t *)(uintptr_t)(target_address + (uint64_t)y * current.pitch +
                                                       (uint64_t)x * bytes_per_pixel),
                    bytes_per_pixel, pack_pixel(red, green, blue));
    }
    return 0;
}

int sb_framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                             uint8_t red, uint8_t green, uint8_t blue) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    uint64_t target_address;
    uint32_t pixel;

    if (width == 0u || height == 0u) return 0;
    if (x >= current.width || y >= current.height) return -1;
    if (width > current.width - x) width = current.width - x;
    if (height > current.height - y) height = current.height - y;

    if (framebuffer_target(&target_address) != 0) return -2;
    pixel = pack_pixel(red, green, blue);

    for (uint32_t row_index = y; row_index < y + height; ++row_index) {
        const uint64_t row_offset = (uint64_t)row_index * current.pitch + (uint64_t)x * bytes_per_pixel;
        volatile uint8_t *row = (volatile uint8_t *)(uintptr_t)(target_address + row_offset);
        if (bytes_per_pixel == 4u) {
            volatile uint32_t *fast = (volatile uint32_t *)(void *)row;
            for (uint32_t column = 0u; column < width; ++column) fast[column] = pixel;
        } else {
            for (uint32_t column = 0u; column < width; ++column)
                store_pixel(row + (uint64_t)column * bytes_per_pixel, bytes_per_pixel, pixel);
        }
    }

    return 0;
}

int sb_framebuffer_draw_glyph8(uint32_t x, uint32_t y, uint64_t bitmap,
                               uint8_t red, uint8_t green, uint8_t blue) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    uint64_t target_address;
    const uint32_t pixel = pack_pixel(red, green, blue);

    if (x > UINT32_MAX - 7u || y > UINT32_MAX - 7u ||
        x >= current.width || y >= current.height) return -1;
    if (framebuffer_target(&target_address) != 0) return -2;
    return draw_glyph8_packed(target_address, bytes_per_pixel, x, y, bitmap, pixel);
}

int sb_framebuffer_draw_glyph8_pair(uint32_t x, uint32_t y,
                                    uint64_t bitmap_a, uint64_t bitmap_b,
                                    uint8_t red, uint8_t green, uint8_t blue) {
    const uint64_t bytes_per_pixel = ((uint64_t)current.bits_per_pixel + 7u) / 8u;
    uint64_t target_address;
    const uint32_t pixel = pack_pixel(red, green, blue);

    if (x > UINT32_MAX - 17u || y > UINT32_MAX - 7u ||
        x >= current.width || y >= current.height ||
        x + 10u >= current.width) return -1;
    if (framebuffer_target(&target_address) != 0) return -2;
    if (draw_glyph8_packed(target_address, bytes_per_pixel, x, y, bitmap_a, pixel) != 0)
        return -3;
    return draw_glyph8_packed(target_address, bytes_per_pixel, x + 10u, y, bitmap_b, pixel);
}
