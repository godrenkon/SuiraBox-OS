# Desktop shell integration layer. GNU make prefers GNUmakefile over Makefile;
# include the canonical rules first, then extend them without rewriting it.
include Makefile

LAUNCHER_OBJ := $(BUILD)/launcher.o
DESKTOP_SHELL_OBJ := $(BUILD)/desktop_shell.o
LAUNCHER_HOST_TEST := $(BUILD)/launcher-host-test

.PHONY: host-launcher-test

$(LAUNCHER_OBJ): userspace/launcher.c userspace/launcher.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(DESKTOP_SHELL_OBJ): userspace/desktop_shell.c userspace/desktop_shell.h userspace/launcher.h userspace/gui.h | $(BUILD)
	$(CC) $(USER_CFLAGS) -Iuserspace -c $< -o $@

$(LAUNCHER_HOST_TEST): tests/launcher_host_test.c userspace/launcher.c userspace/launcher.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/launcher_host_test.c userspace/launcher.c -o $@

host-launcher-test: $(LAUNCHER_HOST_TEST)
	$(LAUNCHER_HOST_TEST)

$(DESKTOP_ELF): $(LAUNCHER_OBJ) $(DESKTOP_SHELL_OBJ)

check: host-launcher-test
