#include "syscall_display.h"
#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "user_access.h"
#include "framebuffer.h"
#include <suirabox/display_abi.h>

_Static_assert(sizeof(sb_display_info_t) == SB_DISPLAY_INFO_SIZE, "display info ABI size");
_Static_assert(sizeof(sb_display_present_t) == SB_DISPLAY_PRESENT_SIZE, "display present ABI size");
_Static_assert(__builtin_offsetof(sb_display_present_t, pixels) == SB_DISPLAY_PRESENT_PIXELS_OFFSET, "display pixels ABI offset");

static sb_irq_frame_t *error(sb_irq_frame_t *frame, int64_t result) {
    frame->rax = (uint64_t)result; return frame;
}
sb_irq_frame_t *sb_syscall_dispatch_display(sb_irq_frame_t *frame) {
    if (frame == 0) return 0;
    sb_task_t *task = scheduler_current();
    sb_process_t *process = task != 0 && task->user_task ? process_get(task->process_id) : 0;
    if (process == 0) return error(frame, SB_SYS_ERROR_INVALID);
    if (frame->rax == SB_SYS_DISPLAY_INFO) {
        if (frame->rdi == 0u || frame->rsi != SB_DISPLAY_INFO_SIZE || frame->rdx != 0u)
            return error(frame, SB_SYS_ERROR_INVALID);
        const sb_framebuffer_info_t *fb = sb_framebuffer_info();
        if (fb == 0 || fb->mapped_address == 0u) return error(frame, SB_SYS_ERROR_NOT_FOUND);
        const sb_display_info_t info = {
            .version = SB_DISPLAY_ABI_VERSION, .width = fb->width, .height = fb->height,
            .pixel_format = SB_DISPLAY_PIXEL_RGB888, .max_present_pixels = SB_DISPLAY_PRESENT_MAX_PIXELS,
        };
        if (user_copy_to(process, frame->rdi, &info, sizeof(info)) != 0) return error(frame, SB_SYS_ERROR_FAULT);
        return error(frame, 0);
    }
    if (frame->rax != SB_SYS_DISPLAY_PRESENT) return error(frame, SB_SYS_ERROR_INVALID);
    /* Bootstrap init currently owns the display. A dedicated compositor
     * capability is future work; children are rejected before pointer access. */
    if (process->pid != 1u) return error(frame, SB_SYS_ERROR_RIGHTS);
    if (frame->rdi == 0u || frame->rsi != SB_DISPLAY_PRESENT_SIZE || frame->rdx != 0u)
        return error(frame, SB_SYS_ERROR_INVALID);
    sb_display_present_t request;
    if (user_copy_from(process, &request, frame->rdi, sizeof(request)) != 0) return error(frame, SB_SYS_ERROR_FAULT);
    if (request.size != sizeof(request) || request.version != SB_DISPLAY_ABI_VERSION || request.flags != 0u ||
        request.width == 0u || request.height == 0u || request.pixels == 0u) return error(frame, SB_SYS_ERROR_INVALID);
    const uint64_t count = (uint64_t)request.width * request.height;
    if (count > SB_DISPLAY_PRESENT_MAX_PIXELS) return error(frame, SB_SYS_ERROR_LIMIT);
    const sb_framebuffer_info_t *fb = sb_framebuffer_info();
    if (fb == 0 || fb->mapped_address == 0u) return error(frame, SB_SYS_ERROR_NOT_FOUND);
    if (request.x >= fb->width || request.y >= fb->height ||
        request.width > fb->width - request.x || request.height > fb->height - request.y)
        return error(frame, SB_SYS_ERROR_INVALID);
    /* Stage the entire bounded source before touching MMIO. A bad mapping at
     * the end of a row cannot partially alter the visible framebuffer. */
    uint32_t pixels[SB_DISPLAY_PRESENT_MAX_PIXELS];
    if (user_copy_from(process, pixels, request.pixels, count * sizeof(uint32_t)) != 0)
        return error(frame, SB_SYS_ERROR_FAULT);
    if (sb_framebuffer_present(request.x, request.y, request.width, request.height, pixels, count) != 0)
        return error(frame, SB_SYS_ERROR_IO);
    return error(frame, 0);
}
