	$(CC) -std=c11 -Wall -Wextra -Werror -Iuserspace tests/desktop_shell_host_test.c userspace/desktop_shell.c userspace/launcher.c userspace/gui.c -o $@
host-desktop-shell-test: $(DESKTOP_SHELL_HOST_TEST)
	$(DESKTOP_SHELL_HOST_TEST)
$(LAUNCHER_HOST_TEST): tests/launcher_host_test.c userspace/launcher.c userspace/launcher.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Iuserspace tests/launcher_host_test.c userspace/launcher.c -o $@
host-launcher-test: $(LAUNCHER_HOST_TEST)
	$(LAUNCHER_HOST_TEST)
$(DEVICE_HOST_TEST): tests/device_host_test.c kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/device_host_test.c kernel/device.c -o $@
host-device-test: $(DEVICE_HOST_TEST)
	$(DEVICE_HOST_TEST)
$(ACPI_HOST_TEST): tests/acpi_host_test.c kernel/acpi.c kernel/acpi.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/acpi_host_test.c kernel/acpi.c kernel/device.c -o $@
host-acpi-test: $(ACPI_HOST_TEST)
	$(ACPI_HOST_TEST)
$(POWER_HOST_TEST): tests/power_host_test.c kernel/power.c kernel/power.h kernel/acpi.c kernel/acpi.h kernel/block.c kernel/block.h kernel/device.c kernel/device.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/power_host_test.c kernel/power.c kernel/acpi.c kernel/block.c kernel/device.c -o $@
host-power-test: $(POWER_HOST_TEST)
	$(POWER_HOST_TEST)
$(BLOCK_HOST_TEST): tests/block_host_test.c kernel/block.c kernel/block.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/block_host_test.c kernel/block.c -o $@
host-block-test: $(BLOCK_HOST_TEST)
	$(BLOCK_HOST_TEST)
$(HARDWARE_SUBSYSTEMS_HOST_TEST): tests/hardware_subsystems_host_test.c kernel/device.c kernel/device.h kernel/usb.c kernel/usb.h kernel/nvme.c kernel/nvme.h kernel/net_device.c kernel/net_device.h kernel/audio.c kernel/audio.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/hardware_subsystems_host_test.c kernel/device.c kernel/usb.c kernel/nvme.c kernel/net_device.c kernel/audio.c -o $@
host-hardware-subsystems-test: $(HARDWARE_SUBSYSTEMS_HOST_TEST)
	$(HARDWARE_SUBSYSTEMS_HOST_TEST)
$(USB_TRANSFER_HOST_TEST): tests/usb_transfer_host_test.c kernel/usb_transfer.c kernel/usb_transfer.h kernel/usb.h | $(BUILD)
	$(CC) -Wall -Wextra -Werror -Ikernel tests/usb_transfer_host_test.c kernel/usb_transfer.c -o $@
host-usb-transfer-test: $(USB_TRANSFER_HOST_TEST)
	$(USB_TRANSFER_HOST_TEST)