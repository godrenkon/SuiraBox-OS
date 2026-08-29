#!/usr/bin/env python3
"""Enforce the SB Desktop built-in/optional resource boundary.

This policy is intentionally conservative: the source repository may contain
small Core data, but large or collection-style payloads must live outside the
OS image. Resource manifests are metadata only and are not installed into the
ISO by the canonical image recipe.
"""

from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD_ISO = ROOT / "build" / "iso"
MAX_UNLISTED_BINARY_BYTES = 256 * 1024
ALLOWLIST = set()
BINARY_SUFFIXES = {
    ".bmp", ".gif", ".ico", ".jpeg", ".jpg", ".mp3", ".ogg", ".png",
    ".wav", ".webp", ".avif", ".zip", ".tar", ".gz", ".xz", ".zst",
    ".7z", ".bin",
}
IGNORED_PARTS = {".git", "build", "out", "dist", "node_modules", "__pycache__"}


def relative(path: Path) -> str:
    return path.as_posix().replace(ROOT.as_posix() + "/", "")


def check_source_binaries() -> list[str]:
    violations: list[str] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(part in IGNORED_PARTS for part in path.parts):
            continue
        if relative(path) in ALLOWLIST or path.suffix.lower() not in BINARY_SUFFIXES:
            continue
        try:
            size = path.stat().st_size
        except OSError as exc:
            violations.append(f"cannot stat {relative(path)}: {exc}")
            continue
        if size > MAX_UNLISTED_BINARY_BYTES:
            violations.append(f"{relative(path)}: {size} bytes")
    return violations


def check_iso_resource_leak() -> list[str]:
    violations: list[str] = []
    if not BUILD_ISO.exists():
        return violations
    resource_root = BUILD_ISO / "resources"
    if resource_root.exists():
        violations.append(f"unexpected Resource payload in ISO: {resource_root.relative_to(ROOT)}")
    for path in BUILD_ISO.rglob("*"):
        if not path.is_file():
            continue
        lower = path.name.lower()
        if any(token in lower for token in ("wallpaper", "iconpack", "themepack", "localepack", "soundpack")):
            violations.append(f"optional-style payload leaked into ISO: {path.relative_to(ROOT)}")
    return violations


def main() -> int:
    violations = check_source_binaries()
    violations.extend(check_iso_resource_leak())
    if violations:
        print("error: core footprint/resource boundary violation:")
        for item in violations:
            print(f"  - {item}")
        return 1

    print("core footprint policy: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
