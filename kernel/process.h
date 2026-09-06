#ifndef SB_KERNEL_PROCESS_H
#define SB_KERNEL_PROCESS_H

#include <stdint.h>
#include "mm/address_space.h"

#define SB_MAX_PROCESSES 32u
#define SB_MAX_THREADS_PER_PROCESS 16u

typedef enum {
    SB_PROCESS_UNUSED = 0,
    SB_PROCESS_CREATED,
    SB_PROCESS_RUNNING,
    SB_PROCESS_SLEEPING,
    SB_PROCESS_EXITED
} sb_process_state_t;

typedef struct {
    uint64_t tid;
    uint64_t runtime_ticks;
    uint32_t priority;
    sb_process_state_t state;
} sb_thread_t;

typedef struct {
    uint64_t pid;
    sb_process_state_t state;
    int64_t exit_code;
    uint32_t thread_count;
    sb_thread_t threads[SB_MAX_THREADS_PER_PROCESS];
    sb_address_space_t address_space;
    uint64_t entry_point;
    uint64_t user_stack_top;
} sb_process_t;

void process_init(void);
sb_process_t *process_create(uint64_t pid);
sb_thread_t *process_create_thread(sb_process_t *process, uint64_t tid, uint32_t priority);
sb_process_t *process_get(uint64_t pid);
uint32_t process_count(void);
int process_activate(sb_process_t *process);
int process_mark_exited(uint64_t pid, int64_t exit_code);
/* Reap EXITED processes whose scheduler tasks are no longer executing.
 * Returns the number of process objects/resources reclaimed. */
uint32_t process_reap_exited(void);
void process_destroy(sb_process_t *process);

#endif
