#ifndef SB_KERNEL_USER_LAUNCH_H
#define SB_KERNEL_USER_LAUNCH_H

#include "process.h"

/* On success this function never returns: it enters the thread at user RIP. */
int process_start_user_thread(sb_process_t *process, sb_thread_t *thread);

#endif
