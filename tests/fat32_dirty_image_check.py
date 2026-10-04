#!/usr/bin/env python3
"""Prove an unsynced create persists only the pre-mutation FAT32 dirty marker."""
from pathlib import Path
import hashlib
import struct
import sys
from fat32_directory_growth_image_check import geometry
from fat32_recovery_image_check import CLEAN, NO_ERROR


def prepare(image: Path, snapshot: Path) -> None:
    expected = bytearray(image.read_bytes())
    _, reserved, copies, fat_size, _, _, _ = geometry(expected)
    for copy in range(copies):
        offset = (reserved + copy * fat_size) * 512 + 4
        value = struct.unpack_from("<I", expected, offset)[0]
        if value & (CLEAN | NO_ERROR) != (CLEAN | NO_ERROR):
            raise ValueError("dirty-session fixture must begin clean in every FAT")
        struct.pack_into("<I", expected, offset, value & ~CLEAN)
    # Compute the expected post-checkpoint image; never mutate the input seed.
    snapshot.write_text(hashlib.sha256(expected).hexdigest() + "\n", encoding="ascii")
    print("FAT32 dirty fixture OK: expected only mirrored status changes")


def verify(image: Path, snapshot: Path) -> None:
    expected = snapshot.read_text(encoding="ascii").strip()
    if len(expected) != 64 or any(c not in "0123456789abcdef" for c in expected):
        raise ValueError("invalid dirty-session baseline")
    if hashlib.sha256(image.read_bytes()).hexdigest() != expected:
        raise ValueError("checkpoint did not persist exactly the dirty marker; unsynced metadata or data changed")
    print("FAT32 dirty image proof OK: durable marker only, unsynced creation not published")


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[1] not in ("prepare", "verify"):
        raise SystemExit("usage: fat32_dirty_image_check.py prepare|verify image snapshot")
    try:
        (prepare if sys.argv[1] == "prepare" else verify)(Path(sys.argv[2]), Path(sys.argv[3]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
