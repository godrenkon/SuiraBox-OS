#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include "../userspace/desktop_shell.h"

int main(void) {
    sb_desktop_shell_t shell;
    const char *activated = 0;

    assert(sb_desktop_shell_click(0, 30, 730, &activated) != 0);
    sb_desktop_shell_init(&shell, 1024u, 768u);
    assert(shell.initialized == 1u);
    assert(sb_desktop_shell_register_default_apps(&shell) == 0);
    assert(shell.launcher.count == 3u);
    assert(sb_desktop_shell_register_default_apps(&shell) != 0);

    assert(sb_desktop_shell_click(&shell, 30, 730, &activated) == 0);
    assert(shell.launcher.open == 1u && activated == 0);
    assert(sb_desktop_shell_key(&shell, 0x50u) == 0);
    assert(shell.launcher.selected == 1u);
    assert(sb_desktop_shell_key(&shell, 0x48u) == 0);
    assert(shell.launcher.selected == 0u);

    assert(sb_desktop_shell_click(&shell, 40, 598, &activated) == 0);
    assert(activated != 0 && activated[0] == 's');
    assert(shell.launcher.open == 0u);

    activated = 0;
    assert(sb_desktop_shell_click(&shell, 300, 700, &activated) != 0);
    assert(activated == 0);

    sb_desktop_shell_init(&shell, 0u, 768u);
    assert(shell.initialized == 0u);
    assert(sb_desktop_shell_register_default_apps(&shell) != 0);

    sb_desktop_shell_init(&shell, 1024u, UINT32_MAX);
    assert(shell.initialized == 1u);
    assert(sb_desktop_shell_click(&shell, 30, 10, &activated) != 0);
    assert(activated == 0);
    return 0;
}
