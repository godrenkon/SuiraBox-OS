#!/bin/sh
set -eu

ROOT=${1:-build/iso}
[ -d "$ROOT" ] || { printf '%s\n' "base image tree not found: $ROOT" >&2; exit 1; }

find "$ROOT" -type f -print | sed "s#^$ROOT/##" | sort > "$ROOT/.base-image-files"

expected='boot/grub/grub.cfg
boot/suirabox.elf
boot/sb-desktop.elf'
printf '%s\n' "$expected" | sort > "$ROOT/.base-image-expected"

if ! diff -u "$ROOT/.base-image-expected" "$ROOT/.base-image-files"; then
    printf '%s\n' 'base image contains an unapproved payload' >&2
    exit 1
fi

for file in "$ROOT"/boot/*; do
    [ -f "$file" ] || continue
    case "$(basename "$file")" in
        suirabox.elf|sb-desktop.elf) ;;
        grub) continue ;;
        *) printf 'unapproved boot payload: %s\n' "$file" >&2; exit 1 ;;
    esac
done

printf '%s\n' 'base image allowlist: OK'
