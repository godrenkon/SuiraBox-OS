#!/usr/bin/env python3
"""Reject obvious accidental large optional assets in the OS source tree.

This is a policy guard, not a complete licensing or dependency scanner. Binary
or generated payloads that belong to Optional Resources must live outside the
OS repository/ISO. Small Core assets are explicitly allow-listed by path or by
size.
"""

from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

# Large binary payloads are presumed optional unless explicitly allow-listed.
MAX_UNLISTED_BINARY_BYTES = 256 * 1024
ALLOWLIST = {
    # Intentionally small and required by the bootstrap/build path.
}

BINARY_SUFFIXES = {
    ".bmp", ".gif", ".ico", ".jpeg", ".jpg", ".mp3", ".ogg", ".png",
    ".wav", ".webp", ".avif", ".zip", ".tar", ".gz", ".xz", ".zst",
    ".7z", ".bin",
}

# Build output, VCS metadata, and caches are not source payloads.
IGNORED_PARTS = {".git", "build", "out", "dist", "node_modules", "__pycache__"}


def main() -> int:
    violations: list[str] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(part in IGNORED_PARTS for part in path.parts):
            continue
        if path.as_posix().replace(ROOT.as_posix() + "/", "") in ALLOWLIST:
            continue
        if path.suffix.lower() not in BINARY_SUFFIXES:
            continue
        try:
            size = path.stat().st_size
        except OSError as exc:
            print(f"error: cannot stat {path}: {exc}")
            return 2
        if size > MAX_UNLISTED_BINARY_BYTES:
            violations.append(f"{path.relative_to(ROOT)}: {size} bytes")

    if violations:
        print("error: large binary payloads must be moved to Optional Resources:")
        for item in violations:
            print(f"  - {item}")
        return 1

    print("core footprint policy: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
