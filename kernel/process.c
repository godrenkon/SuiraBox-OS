#include "process.h"
#include "scheduler.h"
#include "shared_memory.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include <stdint.h>

static sb_process_t processes[SB_MAX_PROCESSES];
static uint32_t process_count_value;
static uint64_t next_pid_value;
static uint64_t next_tid_value;

static void clear_process(sb_process_t *process) {
    if (process == 0) return;
    *process = (sb_process_t){0};
    process->state = SB_PROCESS_UNUSED;
}

static void release_shared_mappings(sb_process_t *process) {
    if (process == 0) return;
    for (uint32_t slot = 0u; slot < SB_MAX_SHARED_MAPPINGS; ++slot) {
        sb_process_shared_mapping_t *mapping = &process->shared_mappings[slot];
        if (mapping->in_use == 0u || mapping->memory == 0) continue;

        const uint32_t pages = mapping->memory->page_count;
        for (uint32_t page = 0u; page < pages; ++page) {
            (void)address_space_unmap_shared_user(
                &process->address_space,
                mapping->base + (uint64_t)page * SB_PAGE_SIZE,
                0);
        }
        sb_shared_memory_release(mapping->memory);
        *mapping = (sb_process_shared_mapping_t){0};
    }
}

static void release_process_resources(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;

    /* Mapping references must disappear while the page tables still exist.
     * Handle callbacks then drop object-owner references, and only afterwards
     * may address-space teardown discard the page-table hierarchy. */
    release_shared_mappings(process);
    (void)sb_handle_close_all(&process->handles);
    address_space_destroy(&process->address_space);
    process->entry_point = 0u;
    process->user_stack_top = 0u;
}

static void collect_process_slot(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    clear_process(process);
    if (process_count_value > 0u) --process_count_value;
}

static int process_tid_in_use(uint64_t tid) {
    if (tid == 0u) return 1;
    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        const sb_process_t *process = &processes[i];
        if (process->state == SB_PROCESS_UNUSED) continue;
        for (uint32_t j = 0u; j < process->thread_count; ++j) {
            if (process->threads[j].tid == tid) return 1;
        }
    }
    return 0;
}

static void advance_pid_floor(uint64_t pid) {
    if (next_pid_value == 0u || pid < next_pid_value) return;
    next_pid_value = pid == UINT64_MAX ? 0u : pid + 1u;
}

static void advance_tid_floor(uint64_t tid) {
    if (next_tid_value == 0u || tid < next_tid_value) return;
    next_tid_value = tid == UINT64_MAX ? 0u : tid + 1u;
}

void process_init(void) {
    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        clear_process(&processes[i]);
    }
    process_count_value = 0u;
    next_pid_value = 2u;
    next_tid_value = 10001u;
}

uint64_t process_allocate_pid(void) {
    while (next_pid_value != 0u) {
        const uint64_t candidate = next_pid_value;
        next_pid_value = candidate == UINT64_MAX ? 0u : candidate + 1u;
        if (process_get(candidate) == 0) return candidate;
    }
    return 0u;
}

uint64_t process_allocate_tid(void) {
    while (next_tid_value != 0u) {
        const uint64_t candidate = next_tid_value;
        next_tid_value = candidate == UINT64_MAX ? 0u : candidate + 1u;
        if (!process_tid_in_use(candidate)) return candidate;
    }
    return 0u;
}

sb_process_t *process_get(uint64_t pid) {
    if (pid == 0u) return 0;
    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED && processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return 0;
}

sb_process_t *process_create(uint64_t pid) {
    if (pid == 0u || process_count_value >= SB_MAX_PROCESSES || process_get(pid) != 0) {
        return 0;
    }

    for (uint32_t i = 0; i < SB_MAX_PROCESSES; ++i) {
        if (processes[i].state != SB_PROCESS_UNUSED) continue;

        clear_process(&processes[i]);
        sb_handle_table_init(&processes[i].handles);
        processes[i].pid = pid;
        processes[i].state = SB_PROCESS_CREATED;
        if (address_space_create(&processes[i].address_space) != 0) {
            clear_process(&processes[i]);
            return 0;
        }
        ++process_count_value;
        advance_pid_floor(pid);
        return &processes[i];
    }

    return 0;
}

sb_thread_t *process_create_thread(sb_process_t *process, uint64_t tid, uint32_t priority) {
    if (process == 0 || tid == 0u || process->thread_count >= SB_MAX_THREADS_PER_PROCESS ||
        process->state == SB_PROCESS_UNUSED || process->state == SB_PROCESS_EXITED ||
        process->state == SB_PROCESS_ZOMBIE || process_tid_in_use(tid)) {
        return 0;
    }

    sb_thread_t *thread = &process->threads[process->thread_count++];
    *thread = (sb_thread_t){0};
    thread->tid = tid;
    thread->priority = priority;
    thread->state = SB_PROCESS_CREATED;
    advance_tid_floor(tid);
    return thread;
}

uint32_t process_count(void) {
    return process_count_value;
}

int process_activate(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED ||
        process->state == SB_PROCESS_EXITED || process->state == SB_PROCESS_ZOMBIE) {
        return -1;
    }
    return address_space_activate(&process->address_space);
}

int process_map_shared_memory(sb_process_t *process,
                              sb_shared_memory_t *memory,
                              uint64_t base,
                              int writable) {
    if (process == 0 || memory == 0 || memory->physical_base == 0u ||
        memory->page_count == 0u || process->state == SB_PROCESS_UNUSED ||
        process->state == SB_PROCESS_EXITED || process->state == SB_PROCESS_ZOMBIE ||
        (base & (SB_PAGE_SIZE - 1u)) != 0u) {
        return -1;
    }

    const uint64_t span = (uint64_t)memory->page_count * SB_PAGE_SIZE;
    if (base < SB_USER_BASE || base >= SB_USER_LIMIT || span == 0u ||
        base > SB_USER_LIMIT - span ||
        ((base >> 39) & 0x1FFu) != SB_USER_PML4_INDEX) {
        return -2;
    }

    sb_process_shared_mapping_t *slot = 0;
    for (uint32_t i = 0u; i < SB_MAX_SHARED_MAPPINGS; ++i) {
        if (process->shared_mappings[i].in_use == 0u) {
            slot = &process->shared_mappings[i];
            break;
        }
    }
    if (slot == 0) return -4;

    const uint64_t flags = SB_VMM_NX | (writable != 0 ? SB_VMM_WRITABLE : 0u);
    uint32_t mapped = 0u;
    for (; mapped < memory->page_count; ++mapped) {
        const uint64_t virtual_address = base + (uint64_t)mapped * SB_PAGE_SIZE;
        const uint64_t physical_address =
            memory->physical_base + (uint64_t)mapped * SB_PAGE_SIZE;
        if (address_space_map_user(&process->address_space,
                                   virtual_address,
                                   physical_address,
                                   flags) != 0) {
            break;
        }
    }
    if (mapped != memory->page_count) {
        while (mapped != 0u) {
            --mapped;
            (void)address_space_unmap_shared_user(
                &process->address_space,
                base + (uint64_t)mapped * SB_PAGE_SIZE,
                0);
        }
        return -3;
    }

    if (sb_shared_memory_retain(memory) != 0) {
        for (uint32_t i = 0u; i < memory->page_count; ++i) {
            (void)address_space_unmap_shared_user(
                &process->address_space,
                base + (uint64_t)i * SB_PAGE_SIZE,
                0);
        }
        return -3;
    }

    slot->base = base;
    slot->size = memory->size;
    slot->memory = memory;
    slot->writable = writable != 0 ? 1u : 0u;
    slot->in_use = 1u;
    return 0;
}

int process_unmap_shared_memory(sb_process_t *process, uint64_t base) {
    if (process == 0 || (base & (SB_PAGE_SIZE - 1u)) != 0u) return -1;

    sb_process_shared_mapping_t *mapping = 0;
    for (uint32_t i = 0u; i < SB_MAX_SHARED_MAPPINGS; ++i) {
        if (process->shared_mappings[i].in_use != 0u &&
            process->shared_mappings[i].base == base) {
            mapping = &process->shared_mappings[i];
            break;
        }
    }
    if (mapping == 0 || mapping->memory == 0) return -2;

    for (uint32_t page = 0u; page < mapping->memory->page_count; ++page) {
        uint64_t physical = 0u;
        const uint64_t virtual_address = base + (uint64_t)page * SB_PAGE_SIZE;
        const uint64_t expected =
            mapping->memory->physical_base + (uint64_t)page * SB_PAGE_SIZE;
        if (address_space_translate_user(&process->address_space,
                                         virtual_address,
                                         &physical) != 0 ||
            (physical & ~(uint64_t)(SB_PAGE_SIZE - 1u)) != expected) {
            return -3;
        }
    }

    for (uint32_t page = 0u; page < mapping->memory->page_count; ++page) {
        const uint64_t virtual_address = base + (uint64_t)page * SB_PAGE_SIZE;
        uint64_t physical = 0u;
        if (address_space_unmap_shared_user(&process->address_space,
                                            virtual_address,
                                            &physical) != 0) {
            return -3;
        }
    }

    sb_shared_memory_release(mapping->memory);
    *mapping = (sb_process_shared_mapping_t){0};
    return 0;
}

int process_mark_exited(uint64_t pid, int64_t exit_code) {
    sb_process_t *process = process_get(pid);
    if (process == 0 || process->state == SB_PROCESS_EXITED ||
        process->state == SB_PROCESS_ZOMBIE) return -1;

    process->exit_code = exit_code;
    process->state = SB_PROCESS_EXITED;
    for (uint32_t i = 0u; i < process->thread_count; ++i) {
        process->threads[i].state = SB_PROCESS_EXITED;
    }
    return 0;
}

int process_wait_child(uint64_t parent_pid,
                       uint64_t child_pid,
                       uint64_t waiter_tid,
                       int64_t *exit_code) {
    if (parent_pid == 0u || child_pid == 0u || waiter_tid == 0u || exit_code == 0) return -1;

    sb_process_t *parent = process_get(parent_pid);
    sb_process_t *child = process_get(child_pid);
    if (parent == 0 || child == 0 || child->parent_pid != parent_pid) return -2;
    if (parent->state == SB_PROCESS_EXITED || parent->state == SB_PROCESS_ZOMBIE) return -3;

    if (child->state == SB_PROCESS_ZOMBIE) {
        *exit_code = child->exit_code;
        process_destroy(child);
        return 0;
    }

    if (child->waiter_tid != 0u && child->waiter_tid != waiter_tid) return -4;
    child->waiter_tid = waiter_tid;
    return 1;
}

int process_cancel_wait(uint64_t child_pid, uint64_t waiter_tid) {
    sb_process_t *child = process_get(child_pid);
    if (child == 0 || waiter_tid == 0u || child->waiter_tid != waiter_tid) return -1;
    child->waiter_tid = 0u;
    return 0;
}

void process_destroy(sb_process_t *process) {
    if (process == 0 || process->state == SB_PROCESS_UNUSED) return;
    release_process_resources(process);
    collect_process_slot(process);
}

uint32_t process_reap_exited(void) {
    uint32_t reaped = 0u;

    for (uint32_t i = 0u; i < SB_MAX_PROCESSES; ++i) {
        sb_process_t *process = &processes[i];
        if (process->state != SB_PROCESS_EXITED) continue;

        const int task_result = scheduler_reap_process(process->pid);
        if (task_result < 0) continue;

        release_process_resources(process);
        ++reaped;

        if (process->waiter_tid != 0u) {
            const uint64_t waiter_tid = process->waiter_tid;
            const uint64_t result = (uint64_t)process->exit_code;
            if (scheduler_wake_task_with_result(waiter_tid, result) == 0) {
                collect_process_slot(process);
                continue;
            }
            process->waiter_tid = 0u;
        }

        process->state = SB_PROCESS_ZOMBIE;
    }

    return reaped;
}
