#!/usr/bin/env python3
"""Seed/check the one CI sector outside the FAT32 volume."""
from pathlib import Path
import struct
import sys

SECTOR_SIZE = 512
VOLUME_SECTORS = 131072
SEED = b"SUIRABOX-DURABILITY-SEED-v1\n".ljust(SECTOR_SIZE, b"\0")
COMMIT = b"SUIRABOX-DURABILITY-COMMIT-v1\n".ljust(SECTOR_SIZE, b"\0")


def check_volume(image, expected_size: int) -> None:
    image.seek(0, 2)
    if image.tell() != expected_size:
        raise ValueError("unexpected durability fixture image size")
    image.seek(0)
    boot = image.read(SECTOR_SIZE)
    if (boot[510:512] != b"\x55\xaa" or
            struct.unpack_from("<H", boot, 11)[0] != SECTOR_SIZE or
            struct.unpack_from("<H", boot, 19)[0] != 0 or
            struct.unpack_from("<I", boot, 32)[0] != VOLUME_SECTORS):
        raise ValueError("fixture must be an exact 64 MiB FAT32 volume")


def seed(path: Path) -> None:
    with path.open("r+b") as image:
        check_volume(image, VOLUME_SECTORS * SECTOR_SIZE)
        image.seek(VOLUME_SECTORS * SECTOR_SIZE)
        image.write(SEED)


def verify(path: Path) -> None:
    with path.open("rb") as image:
        check_volume(image, (VOLUME_SECTORS + 1) * SECTOR_SIZE)
        image.seek(VOLUME_SECTORS * SECTOR_SIZE)
        if image.read(SECTOR_SIZE) != COMMIT:
            raise ValueError("durability sector did not persist the complete commit")
    print("storage image durability proof OK: persisted sector after QEMU exit")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("seed", "verify"):
        raise SystemExit("usage: storage_image_check.py seed|verify IMAGE")
    try:
        (seed if sys.argv[1] == "seed" else verify)(Path(sys.argv[2]))
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
