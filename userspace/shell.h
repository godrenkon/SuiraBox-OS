#ifndef SB_SHELL_H
#define SB_SHELL_H

#include <stdint.h>

/* Stable userspace shell command ABI. */
uint64_t sb_shell_command_count(void);
const char *sb_shell_command_name(uint64_t index);
uint64_t sb_shell_run_command(uint64_t index);

#endif
