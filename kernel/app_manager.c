#include "app_manager.h"
#include "process.h"
#include "process_exec.h"
#include "mm/multiboot_modules.h"
#include "user_scheduler.h"

#define SB_APP_MAX_RUNNING 8u
#define SB_APP_PID_BASE 0x1000u
#define SB_APP_TID_BASE 0x2000u

typedef struct {
    uint8_t active;
    uint32_t app_id;
    sb_process_t *process;
    sb_user_context_t context;
    sb_thread_t *thread;
} sb_app_instance_t;

static uint64_t multiboot_info_value;
static uint64_t next_pid = SB_APP_PID_BASE;
static uint64_t next_tid = SB_APP_TID_BASE;
static sb_app_instance_t instances[SB_APP_MAX_RUNNING];

static const char *module_name_for_app(uint32_t app_id) {
    switch (app_id) {
        case SB_APP_SETTINGS: return "sb-app-settings";
        case SB_APP_FILES: return "sb-app-files";
        case SB_APP_TERMINAL: return "sb-app-terminal";
        default: return 0;
    }
}

static int app_id_is_valid(uint32_t app_id) {
    return module_name_for_app(app_id) != 0;
}

static int app_already_running(uint32_t app_id) {
    for (uint32_t i = 0u; i < SB_APP_MAX_RUNNING; ++i)
        if (instances[i].active != 0u && instances[i].app_id == app_id) return 1;
    return 0;
}

void sb_app_manager_init(uint64_t multiboot_info) {
    multiboot_info_value = multiboot_info;
    next_pid = SB_APP_PID_BASE;
    next_tid = SB_APP_TID_BASE;
    for (uint32_t i = 0u; i < SB_APP_MAX_RUNNING; ++i) instances[i] = (sb_app_instance_t){0};
}

int sb_app_launch(uint32_t app_id) {
    const char *module_name = module_name_for_app(app_id);
    if (!app_id_is_valid(app_id) || multiboot_info_value == 0u || app_already_running(app_id)) return -1;

    uint32_t slot = SB_APP_MAX_RUNNING;
    for (uint32_t i = 0u; i < SB_APP_MAX_RUNNING; ++i) {
        if (instances[i].active == 0u) { slot = i; break; }
    }
    if (slot == SB_APP_MAX_RUNNING) return -1;

    sb_process_t *process = process_create(next_pid++);
    if (process == 0) return -1;
    sb_process_image_t image;
    if (process_prepare_boot_module(process, multiboot_info_value, module_name, &image) != 0) {
        process_destroy(process);
        return -1;
    }
    sb_user_context_t *context = &instances[slot].context;
    sb_thread_t *thread = 0;
    if (process_prepare_elf_thread(process, next_tid++, 128u, context, &image, &thread) != 0) {
        process_destroy(process);
        return -1;
    }
    if (user_scheduler_add(process, thread) != 0) {
        process_destroy(process);
        return -1;
    }

    instances[slot].active = 1u;
    instances[slot].app_id = app_id;
    instances[slot].process = process;
    instances[slot].thread = thread;
    return 0;
}

uint32_t sb_app_count(void) {
    uint32_t count = 0u;
    for (uint32_t i = 0u; i < SB_APP_MAX_RUNNING; ++i) count += instances[i].active != 0u ? 1u : 0u;
    return count;
}
