#include <assert.h>
#include <stdint.h>
#include "../kernel/app_manager.h"
#include "../kernel/process.h"
#include "../kernel/process_exec.h"
#include "../kernel/user_scheduler.h"

static sb_process_t processes[16];
static sb_thread_t threads[16];
static uint32_t process_count_value;
static uint32_t prepare_calls;
static uint32_t scheduler_calls;
static int fail_prepare;
static int fail_scheduler;

sb_process_t *process_create(uint64_t pid) {
    if (process_count_value >= 16u) return 0;
    sb_process_t *process = &processes[process_count_value++];
    *process = (sb_process_t){ .pid = pid, .state = SB_PROCESS_CREATED };
    return process;
}

void process_destroy(sb_process_t *process) {
    if (process == 0) return;
    process->state = SB_PROCESS_UNUSED;
}

int process_prepare_boot_module(sb_process_t *process, uint64_t multiboot_info,
                                const char *module_name, sb_process_image_t *image) {
    (void)multiboot_info;
    ++prepare_calls;
    if (fail_prepare != 0 || process == 0 || module_name == 0 || image == 0) return -1;
    image->entry_point = 0x400000u;
    image->image_base = 0x200000u;
    image->image_size = 0x1000u;
    return 0;
}

int process_prepare_elf_thread(sb_process_t *process, uint64_t tid, uint32_t priority,
                               sb_user_context_t *context, const sb_process_image_t *image,
                               sb_thread_t **thread_out) {
    (void)priority;
    if (process == 0 || context == 0 || image == 0 || thread_out == 0) return -1;
    if (process->pid < 16u) {
        sb_thread_t *thread = &threads[process->pid];
        *thread = (sb_thread_t){ .tid = tid, .state = SB_PROCESS_CREATED,
                                 .user_context = context,
                                 .kernel_stack_base = 0x10000u + process->pid * 0x1000u,
                                 .kernel_stack_top = 0x10800u + process->pid * 0x1000u,
                                 .kernel_resume_stack_pointer = 0x10400u + process->pid * 0x1000u };
        process->threads[0] = *thread;
        process->thread_count = 1u;
        *thread_out = thread;
        return 0;
    }
    return -1;
}

int user_scheduler_add(sb_process_t *process, sb_thread_t *thread) {
    ++scheduler_calls;
    return fail_scheduler != 0 || process == 0 || thread == 0 ? -1 : 0;
}

int main(void) {
    sb_app_manager_init(0x1000u);
    assert(sb_app_count() == 0u);
    assert(sb_app_launch(0u) != 0);
    assert(sb_app_launch(99u) != 0);

    fail_prepare = 1;
    assert(sb_app_launch(SB_APP_SETTINGS) != 0);
    assert(prepare_calls == 1u);
    fail_prepare = 0;

    assert(sb_app_launch(SB_APP_SETTINGS) == 0);
    assert(sb_app_count() == 1u);
    assert(sb_app_launch(SB_APP_SETTINGS) != 0);
    assert(prepare_calls == 3u);
    assert(scheduler_calls == 2u);

    assert(sb_app_launch(SB_APP_FILES) == 0);
    assert(sb_app_launch(SB_APP_TERMINAL) == 0);
    assert(sb_app_count() == 3u);

    fail_scheduler = 1;
    assert(sb_app_launch(4u) != 0);
    fail_scheduler = 0;
    assert(sb_app_count() == 3u);

    assert(sb_app_launch(SB_APP_TERMINAL) != 0);
    assert(sb_app_count() == 3u);

    return 0;
}
