#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include "../userspace/shell.h"

uint64_t sb_get_ticks(void) {
    return 0x12345678u;
}

uint64_t sb_process_id(void) {
    return 42u;
}

int main(void) {
    assert(sb_shell_command_count() == 2u);
    assert(sb_shell_command_name(0u) != 0);
    assert(sb_shell_command_name(1u) != 0);
    assert(sb_shell_command_name(2u) == 0);
    assert(sb_shell_command_name(0u)[0] == 't');
    assert(sb_shell_command_name(1u)[0] == 'p');
    assert(sb_shell_run_command(0u) == 0x12345678u);
    assert(sb_shell_run_command(1u) == 42u);
    assert(sb_shell_run_command(2u) == UINT64_MAX);
    assert(sb_shell_run_command(UINT64_MAX) == UINT64_MAX);
    return 0;
}
