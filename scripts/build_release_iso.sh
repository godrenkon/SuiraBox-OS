#!/bin/sh
set -eu

# Release build: retain runtime functionality while removing boot self-tests and
# test-only storage code. Development CI keeps those checks enabled.
BASE_CFLAGS='-ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident'
USER_CFLAGS='-ffreestanding -fno-stack-protector -fno-pie -fno-builtin -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident'

rm -rf build
make -f GNUmakefile \
    STORAGE_TEST_OBJ= \
    CFLAGS="$BASE_CFLAGS -DSB_BOOT_SELFTESTS=0 -Dsb_storage_selftest()=1 -Dvmm_selftest()=1 -Dheap_selftest()=1 -Dscheduler_selftest()=1 -Dprocess_syscall_selftest()=1" \
    USER_CFLAGS="$USER_CFLAGS" \
    iso

# Strip linker-only symbols from the release images. Runtime load segments are
# preserved; this only removes non-runtime symbol tables/metadata.
strip --strip-all build/suirabox.elf build/sb-desktop.elf

sh scripts/check_base_image.sh build/iso

printf 'Release ISO: %s bytes\n' "$(wc -c < build/suirabox.iso)"
printf 'Kernel ELF: %s bytes\n' "$(wc -c < build/suirabox.elf)"
printf 'Desktop ELF: %s bytes\n' "$(wc -c < build/sb-desktop.elf)"
