#include "process.h"
#include "mm/pmm.h"
#include "arch/x86_64/irq_frame.h"
#include "user_scheduler.h"
#include "fs_syscall.h"

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;

static int process_pid_in_use(uint64_t pid) {
    if (pid == 0u) return 1;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i)
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) return 1;
    return 0;
}

static void clear_kernel_stack(uint8_t *stack) {
    for (uint32_t i = 0u; i < SB_USER_KERNEL_STACK_SIZE; ++i) stack[i] = 0u;
}

void process_init(void) {
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        processes[i] = (sb_process_t){0};
        processes[i].state = SB_PROCESS_UNUSED;
    }
    process_count_value = 0u;
}

sb_process_t *process_create(uint64_t pid) {
    if (process_count_value >= SB_MAX_PROCESSES || process_pid_in_use(pid)) return 0;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED) continue;
        processes[i] = (sb_process_t){0};
        processes[i].pid = pid;
        processes[i].parent_pid = 0u;
        processes[i].state = SB_PROCESS_CREATED;
        if (address_space_create(&processes[i].address_space) != 0) {
            processes[i].state = SB_PROCESS_UNUSED;
            processes[i].pid = 0u;
            return 0;
        }
        ++process_count_value;
        return &processes[i];
    }
    return 0;
}

sb_process_t *process_create_child(sb_process_t *parent, uint64_t pid) {
    sb_process_t *child;
    if (parent == 0 || parent->state == SB_PROCESS_UNUSED || parent->state == SB_PROCESS_EXITED) return 0;
    child = process_create(pid);
    if (child == 0) return 0;
    child->parent_pid = parent->pid;
    return child;
}

sb_thread_t *process_create_thread(sb_process_t *process, uint64_t tid, uint32_t priority) {
    if (process == 0 || tid == 0u || process->thread_count >= SB_MAX_THREADS_PER_PROCESS ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) return 0;
    for (uint32_t i = 0u; i < SB_MAX_THREADS_PER_PROCESS; ++i)
        if (process->threads[i].tid == tid) return 0;

    void *stack = pmm_alloc_page();
    if (stack == 0) return 0;
    clear_kernel_stack((uint8_t *)stack);

    sb_thread_t *thread = &process->threads[process->thread_count];
    *thread = (sb_thread_t){0};
    thread->tid = tid;
    thread->priority = priority;
    thread->state = SB_PROCESS_CREATED;
    thread->kernel_stack_base = (uint64_t)(uintptr_t)stack;
    thread->kernel_stack_top = thread->kernel_stack_base + SB_USER_KERNEL_STACK_SIZE;
    ++process->thread_count;
    return thread;
}

int process_destroy_thread(sb_process_t *process, sb_thread_t *thread) {
    uint32_t index = process != 0 ? process->thread_count : 0u;
    if (process == 0 || thread == 0 || process->thread_count == 0u) return -1;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        if (&process->threads[i] == thread) {
            index = i;
            break;
        }
    }
    if (index >= process->thread_count) return -1;
    const uint64_t removed_stack = thread->kernel_stack_base;
    (void)user_scheduler_remove(process, thread);
    const uint32_t last_index = process->thread_count - 1u;
    if (index != last_index) {
        sb_thread_t *last_thread = &process->threads[last_index];
        process->threads[index] = *last_thread;
        (void)user_scheduler_rebind_thread(process, last_thread, &process->threads[index]);
    } else {
        process->threads[last_index] = (sb_thread_t){0};
    }
    if (removed_stack != 0u) pmm_free_page((void *)(uintptr_t)removed_stack);
    --process->thread_count;
    return 0;
}

int process_prepare_thread_context(sb_thread_t *thread, sb_user_context_t *context,
                                   uint64_t entry_point, uint64_t user_stack_top) {
    if (thread == 0 || context == 0 || thread->tid == 0u ||
        thread->state == SB_PROCESS_UNUSED || thread->state == SB_PROCESS_EXITED ||
        thread->kernel_stack_base == 0u || thread->kernel_stack_top == 0u) return -1;
    if (sb_user_context_init(context, entry_point, user_stack_top) != 0) return -1;
    thread->user_context = context;
    if (process_prepare_user_resume_frame(thread) != 0) {
        thread->user_context = 0;
        thread->kernel_resume_stack_pointer = 0u;
        return -1;
    }
    return 0;
}

int process_prepare_user_resume_frame(sb_thread_t *thread) {
    if (thread == 0 || thread->user_context == 0 || thread->kernel_stack_base == 0u ||
        thread->kernel_stack_top == 0u || thread->kernel_stack_top < thread->kernel_stack_base ||
        thread->kernel_stack_top - thread->kernel_stack_base < SB_USER_RESUME_FRAME_OFFSET) return -1;
    if (sb_user_context_validate(thread->user_context) != 0) return -1;
    const uint64_t frame_address = thread->kernel_stack_top - SB_USER_RESUME_FRAME_OFFSET;
    if (frame_address < thread->kernel_stack_base ||
        thread->kernel_stack_top - frame_address < SB_USER_RESUME_FRAME_SIZE ||
        (frame_address & 0xFu) != 0u) return -1;
    sb_timer_saved_gpr_t *gpr = (sb_timer_saved_gpr_t *)(uintptr_t)frame_address;
    sb_x86_64_user_iret_frame_t *iret =
        (sb_x86_64_user_iret_frame_t *)(uintptr_t)(frame_address + sizeof(*gpr));
    const sb_user_context_t *context = thread->user_context;
    *gpr = (sb_timer_saved_gpr_t){
        .r15=context->r15, .r14=context->r14, .r13=context->r13, .r12=context->r12,
        .rbp=context->rbp, .rbx=context->rbx, .r11=context->r11, .r10=context->r10,
        .r9=context->r9, .r8=context->r8, .rdi=context->rdi, .rsi=context->rsi,
        .rdx=context->rdx, .rcx=context->rcx, .rax=context->rax
    };
    *iret = (sb_x86_64_user_iret_frame_t){
        .rip=context->rip, .cs=context->cs, .rflags=context->rflags,
        .rsp=context->rsp, .ss=context->ss
    };
    thread->kernel_resume_stack_pointer = frame_address;
    return 0;
}

static int thread_belongs_to_process(const sb_process_t *process, const sb_thread_t *thread) {
    if (process == 0 || thread == 0) return 0;
    for (uint32_t i = 0u; i < process->thread_count; ++i)
        if (&process->threads[i] == thread) return 1;
    return 0;
}

static uint32_t runnable_thread_count(const sb_process_t *process) {
    uint32_t count = 0u;
    if (process == 0) return 0u;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        const sb_thread_t *thread = &process->threads[i];
        if (thread->state == SB_PROCESS_CREATED || thread->state == SB_PROCESS_RUNNING) ++count;
    }
    return count;
}

int process_exit_thread(sb_process_t *process, sb_thread_t *thread, uint64_t exit_code) {
    if (!thread_belongs_to_process(process, thread) || process->state == SB_PROCESS_UNUSED ||
        process->state == SB_PROCESS_EXITED || thread->state == SB_PROCESS_UNUSED ||
        thread->state == SB_PROCESS_EXITED) return -1;
    thread->state = SB_PROCESS_EXITED;
    thread->runtime_ticks = 0u;
    if (user_scheduler_current_process() == process && user_scheduler_current_thread() == thread) {
        if (user_scheduler_request_exit(process, thread) != 0) {
            thread->state = SB_PROCESS_RUNNING;
            return -1;
        }
    } else {
        (void)user_scheduler_remove(process, thread);
    }
    if (runnable_thread_count(process) == 0u) {
        process->state = SB_PROCESS_EXITED;
        process->exit_code = exit_code;
        sb_fs_release_process(process);
    }
    return 0;
}

int process_terminate(sb_process_t *process, uint64_t exit_code) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) return -1;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        process->threads[i].state = SB_PROCESS_EXITED;
        if (user_scheduler_current_process() == process && user_scheduler_current_thread() == &process->threads[i])
            (void)user_scheduler_request_exit(process, &process->threads[i]);
        else
            (void)user_scheduler_remove(process, &process->threads[i]);
    }
    process->state = SB_PROCESS_EXITED;
    process->exit_code = exit_code;
    sb_fs_release_process(process);
    return 0;
}

uint64_t process_wait_child(sb_process_t *parent, uint64_t child_pid, uint64_t *exit_code) {
    if (parent == 0 || parent->state == SB_PROCESS_UNUSED) return UINT64_MAX;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        sb_process_t *child = &processes[i];
        if (child->state == SB_PROCESS_UNUSED || child->parent_pid != parent->pid) continue;
        if (child_pid != 0u && child->pid != child_pid) continue;
        if (child->state != SB_PROCESS_EXITED) return 0u;
        if (exit_code != 0) *exit_code = child->exit_code;
        const uint64_t result_pid = child->pid;
        process_destroy(child);
        return result_pid;
    }
    return UINT64_MAX;
}

sb_process_t *process_get(uint64_t pid) {
    if (pid == 0u) return 0;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i)
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) return &processes[i];
    return 0;
}

uint32_t process_count(void) { return process_count_value; }

int process_activate(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED) return -1;
    if (process->address_space.pml4_physical == 0u) return -1;
    return address_space_activate(&process->address_space);
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    sb_fs_release_process(process);
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        (void)user_scheduler_remove(process, &process->threads[i]);
        if (process->threads[i].kernel_stack_base != 0u)
            pmm_free_page((void *)(uintptr_t)process->threads[i].kernel_stack_base);
    }
    address_space_destroy(&process->address_space);
    *process = (sb_process_t){0};
    process->state = SB_PROCESS_UNUSED;
    if (process_count_value > 0u) --process_count_value;
}
