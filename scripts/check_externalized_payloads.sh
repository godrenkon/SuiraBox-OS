#!/bin/sh
set -eu

max_bytes=$((512 * 1024))
status=0

while IFS= read -r -d '' path; do
    case "$path" in
        .git/*|build/*) continue ;;
    esac

    case "$path" in
        *.png|*.jpg|*.jpeg|*.webp|*.gif|*.bmp|*.wav|*.mp3|*.ogg|*.flac|*.m4a|*.aac|*.ttf|*.otf|*.woff|*.woff2|*.zip|*.7z|*.rar|*.tar|*.tar.gz|*.tgz|*.tar.zst|*.zst)
            printf 'external resource payload must not be vendored: %s\n' "$path" >&2
            status=1
            ;;
    esac

    size=$(wc -c < "$path")
    if [ "$size" -gt "$max_bytes" ]; then
        printf 'large tracked file exceeds base-source budget (%s bytes): %s\n' "$size" "$path" >&2
        status=1
    fi
done <<EOF
$(git ls-files -z | tr '\0' '\n')
EOF

exit "$status"
