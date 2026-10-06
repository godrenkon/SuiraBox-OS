#include "framebuffer.h"
#include "display_surface.h"
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

static sb_display_surface_t surface_at(uint64_t address) {
    return (sb_display_surface_t){
        .pixels = (volatile uint8_t *)(uintptr_t)address,
        .byte_length = (uint64_t)current.pitch * current.height,
        .width = current.width, .height = current.height, .pitch = current.pitch,
        .bits_per_pixel = current.bits_per_pixel,
        .red_position = current.red_position, .red_size = current.red_mask_size,
        .green_position = current.green_position, .green_size = current.green_mask_size,
        .blue_position = current.blue_position, .blue_size = current.blue_mask_size,
    };
}

static int framebuffer_geometry_ok(void) {
    if (current.type != SB_FB_DIRECT) return 0;
    const sb_display_surface_t surface = surface_at(current.address);
    return sb_display_surface_valid(&surface);
}

static int reject_framebuffer(void) {
    available = 0;
    current = (sb_framebuffer_info_t){0};
    return 0;
}

static int framebuffer_target(uint64_t *target_address) {
    if (target_address == 0 || !available || !framebuffer_geometry_ok()) return -1;
    if (current.mapped_address != 0u) {
        if (current.mapped_size < (uint64_t)current.pitch * current.height) return -2;
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

int sb_framebuffer_init(uint64_t multiboot_info_address) {
    available = 0;
    current = (sb_framebuffer_info_t){0};

    if (multiboot_info_address == 0u || (multiboot_info_address & 7u) != 0u || multiboot_info_address >= 0x40000000ull)
        return reject_framebuffer();

    {
        const uint32_t total_size = *(const uint32_t *)(uintptr_t)multiboot_info_address;
        if (total_size < 16u || total_size > SB_MB2_MAX_INFO_SIZE ||
            (total_size & 7u) != 0u || total_size > SB_IDENTITY_MAP_LIMIT - multiboot_info_address)
            return reject_framebuffer();

        uint32_t offset = 8u;
        uint32_t tags_seen = 0u;
        int ended = 0;
        while (offset <= total_size - 8u && tags_seen++ < SB_MB2_MAX_TAGS) {
            const sb_mb2_tag_t *tag = (const sb_mb2_tag_t *)(uintptr_t)(multiboot_info_address + offset);
            if (tag->size < 8u || tag->size > total_size - offset) return reject_framebuffer();
            if (tag->type == SB_MB2_TAG_END) {
                if (tag->size != 8u || offset + 8u != total_size) return reject_framebuffer();
                ended = 1;
                break;
            }

            if (tag->type == SB_MB2_TAG_FRAMEBUFFER) {
                if (available || tag->size < sizeof(sb_mb2_fb_tag_prefix_t)) return reject_framebuffer();
                const sb_mb2_fb_tag_prefix_t *fb = (const sb_mb2_fb_tag_prefix_t *)tag;
                const uint64_t bytes_per_pixel = ((uint64_t)fb->bpp + 7u) / 8u;
                const uint64_t total_bytes = (uint64_t)fb->pitch * fb->height;
                if (fb->address != 0u && fb->pitch != 0u && fb->width != 0u &&
                    fb->height != 0u && fb->bpp != 0u && fb->bpp <= SB_FB_MAX_BPP &&
                    bytes_per_pixel <= 4u && total_bytes <= UINT64_MAX - fb->address) {
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
                if (!available) return reject_framebuffer();
            }

            {
                const uint64_t next = align8(tag->size);
                if (next > UINT32_MAX || next > (uint64_t)(total_size - offset)) return reject_framebuffer();
                offset += (uint32_t)next;
            }
        }
        if (!ended) return reject_framebuffer();
    }

    return available;
}

int sb_framebuffer_available(void) { return available; }

const sb_framebuffer_info_t *sb_framebuffer_info(void) {
    return available ? &current : (const sb_framebuffer_info_t *)0;
}

int sb_framebuffer_map(void) {
    uint64_t physical_start, physical_end, page_count, virtual_start;

    if (!available || !framebuffer_geometry_ok()) return 0;
    if (current.mapped_address != 0u) return 1;

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

static int drawing_surface(sb_display_surface_t *surface) {
    uint64_t address;
    if (framebuffer_target(&address) != 0) return -1;
    *surface = surface_at(address);
    return sb_display_surface_valid(surface) ? 0 : -1;
}

int sb_framebuffer_clear(uint8_t r, uint8_t g, uint8_t b) {
    return sb_framebuffer_fill_rect(0u, 0u, current.width, current.height, r, g, b);
}
int sb_framebuffer_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    return sb_framebuffer_fill_rect(x, y, 1u, 1u, r, g, b);
}
int sb_framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                             uint8_t r, uint8_t g, uint8_t b) {
    sb_display_surface_t surface;
    if (drawing_surface(&surface) != 0) return -1;
    return sb_display_surface_fill(&surface, x, y, width, height, r, g, b);
}
int sb_framebuffer_present(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                            const uint32_t *rgb, uint64_t count) {
    sb_display_surface_t surface;
    if (drawing_surface(&surface) != 0) return -1;
    return sb_display_surface_present(&surface, x, y, width, height, rgb, count);
}
