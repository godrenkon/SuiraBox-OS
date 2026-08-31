#!/bin/sh
set -eu

# Release build: use the minimal GUI kernel entrypoint instead of the
# diagnostic development kernel. Development CI keeps the full self-tests.
CFLAGS_RELEASE='-ffreestanding -fno-stack-protector -fno-pie -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -DSB_KERNEL_DEBUG=0'
USER_CFLAGS_RELEASE='-ffreestanding -fno-stack-protector -fno-pie -fno-builtin -mno-red-zone -m64 -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident'

rm -rf build
make -f GNUmakefile \
    CFLAGS="$CFLAGS_RELEASE" \
    USER_CFLAGS="$USER_CFLAGS_RELEASE" \
    release-iso

# Release artifacts must not retain development symbol tables. Strip both
# the build outputs and the copies already staged into the release tree,
# then rebuild the ISO so the shipped payload is stripped as well.
strip --strip-all build/suirabox-release.elf
strip --strip-all build/sb-desktop.elf
strip --strip-all build/release-iso/boot/suirabox.elf
strip --strip-all build/release-iso/boot/sb-desktop.elf
rm -f build/suirabox-release.iso
grub-mkrescue -o build/suirabox-release.iso build/release-iso >/dev/null

sh scripts/check_base_image.sh build/release-iso

printf 'Release ISO: %s bytes\n' "$(wc -c < build/suirabox-release.iso)"
printf 'Release kernel ELF: %s bytes\n' "$(wc -c < build/suirabox-release.elf)"
printf 'Release desktop ELF: %s bytes\n' "$(wc -c < build/sb-desktop.elf)"
