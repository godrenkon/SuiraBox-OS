#ifndef SB_SHELL_H
#define SB_SHELL_H

#include <stdint.h>

/* Stable userspace shell command ABI. */
uint64_t sb_shell_command_count(void);
uint64_t sb_shell_run_command(uint64_t index);

#endif
