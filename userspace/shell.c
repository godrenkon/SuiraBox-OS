#include <stdint.h>
#include "syscall.h"
#include "shell.h"

/* Minimal userspace command dispatcher. Console I/O will be supplied by the
 * terminal device layer; for now these handlers expose the process/runtime ABI. */
typedef uint64_t (*sb_command_fn)(void);

typedef struct {
    const char *name;
    sb_command_fn run;
} sb_command_t;

static uint64_t cmd_ticks(void) {
    return sb_get_ticks();
}

static uint64_t cmd_pid(void) {
    return sb_process_id();
}

static const sb_command_t commands[] = {
    {"ticks", cmd_ticks},
    {"pid", cmd_pid},
};

uint64_t sb_shell_command_count(void) {
    return sizeof(commands) / sizeof(commands[0]);
}

uint64_t sb_shell_run_command(uint64_t index) {
    if (index >= sb_shell_command_count()) return UINT64_MAX;
    return commands[index].run();
}
