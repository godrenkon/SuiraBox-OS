#ifndef SB_KERNEL_PROCESS_H
#define SB_KERNEL_PROCESS_H

#include <stdint.h>
#include "handle.h"
#include "mm/address_space.h"

#define SB_MAX_PROCESSES 32u
#define SB_MAX_THREADS_PER_PROCESS 16u
#define SB_MAX_SHARED_MAPPINGS 16u

typedef struct sb_shared_memory sb_shared_memory_t;

typedef enum {
    SB_PROCESS_UNUSED = 0,
    SB_PROCESS_CREATED,
    SB_PROCESS_RUNNING,
    SB_PROCESS_SLEEPING,
    SB_PROCESS_EXITED,
    SB_PROCESS_ZOMBIE
} sb_process_state_t;

typedef struct {
    uint64_t tid;
    uint64_t runtime_ticks;
    uint32_t priority;
    sb_process_state_t state;
} sb_thread_t;

typedef struct {
    uint64_t base;
    uint64_t size;
    sb_shared_memory_t *memory;
    uint8_t writable;
    uint8_t in_use;
} sb_process_shared_mapping_t;

typedef struct {
    uint64_t pid;
    uint64_t parent_pid;
    uint64_t waiter_tid;
    sb_process_state_t state;
    int64_t exit_code;
    uint32_t thread_count;
    sb_thread_t threads[SB_MAX_THREADS_PER_PROCESS];
    sb_address_space_t address_space;
    sb_handle_table_t handles;
    sb_process_shared_mapping_t shared_mappings[SB_MAX_SHARED_MAPPINGS];
    uint64_t entry_point;
    uint64_t user_stack_top;
} sb_process_t;

void process_init(void);

/* Allocate globally unique identifiers among live process/thread objects.
 * Identifiers are monotonic for the current boot and are never zero. */
uint64_t process_allocate_pid(void);
uint64_t process_allocate_tid(void);

sb_process_t *process_create(uint64_t pid);
sb_thread_t *process_create_thread(sb_process_t *process, uint64_t tid, uint32_t priority);
sb_process_t *process_get(uint64_t pid);
uint32_t process_count(void);
int process_activate(sb_process_t *process);
int process_mark_exited(uint64_t pid, int64_t exit_code);

/* Shared mappings hold references independently from process-local handles.
 * This lets a handle close without invalidating an established mapping and
 * lets process teardown release mappings before its address space disappears. */
int process_map_shared_memory(sb_process_t *process,
                              sb_shared_memory_t *memory,
                              uint64_t base,
                              int writable);
int process_unmap_shared_memory(sb_process_t *process, uint64_t base);

/* Wait return values: 0 = already collected and exit_code filled,
 * 1 = waiter registered and caller must BLOCK+reschedule, <0 = invalid. */
int process_wait_child(uint64_t parent_pid,
                       uint64_t child_pid,
                       uint64_t waiter_tid,
                       int64_t *exit_code);
int process_cancel_wait(uint64_t child_pid, uint64_t waiter_tid);

/* Reap EXITED processes whose scheduler tasks are no longer executing.
 * Scheduler-owned stacks are released first, then process-local mappings,
 * handles and address-space resources. */
uint32_t process_reap_exited(void);
void process_destroy(sb_process_t *process);

#endif
