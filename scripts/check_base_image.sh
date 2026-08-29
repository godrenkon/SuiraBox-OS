#!/bin/sh
set -eu

ROOT=${1:-build/iso}
[ -d "$ROOT" ] || { printf '%s\n' "base image tree not found: $ROOT" >&2; exit 1; }

ACTUAL=$(mktemp)
EXPECTED=$(mktemp)
trap 'rm -f "$ACTUAL" "$EXPECTED"' EXIT HUP INT TERM

find "$ROOT" -type f -print | sed "s#^$ROOT/##" | sort > "$ACTUAL"
printf '%s\n' 'boot/grub/grub.cfg' 'boot/suirabox.elf' 'boot/sb-desktop.elf' | sort > "$EXPECTED"

if ! diff -u "$EXPECTED" "$ACTUAL"; then
    printf '%s\n' 'base image contains an unapproved payload' >&2
    exit 1
fi

printf '%s\n' 'base image allowlist: OK'
