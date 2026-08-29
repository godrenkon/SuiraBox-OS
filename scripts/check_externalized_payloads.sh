#!/bin/sh
set -eu

max_bytes=$((512 * 1024))
status=0
paths=$(mktemp)
trap 'rm -f "$paths"' EXIT HUP INT TERM

git ls-files > "$paths"
while IFS= read -r path; do
    case "$path" in
        .git/*|build/*) continue ;;
    esac

    case "$path" in
        *.png|*.jpg|*.jpeg|*.webp|*.gif|*.bmp|*.wav|*.mp3|*.ogg|*.flac|*.m4a|*.aac|*.ttf|*.otf|*.woff|*.woff2|*.zip|*.7z|*.rar|*.tar|*.tar.gz|*.tgz|*.tar.zst|*.zst)
            printf 'external resource payload must not be vendored: %s\n' "$path" >&2
            status=1
            ;;
    esac

    size=$(git cat-file -s ":$path")
    if [ "$size" -gt "$max_bytes" ]; then
        printf 'large tracked file exceeds base-source budget (%s bytes): %s\n' "$size" "$path" >&2
        status=1
    fi
done < "$paths"

exit "$status"
