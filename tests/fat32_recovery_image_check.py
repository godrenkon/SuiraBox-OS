#!/usr/bin/env python3
"""Set FAT[1] recovery evidence in a named CI fixture and reject any boot writes."""
from pathlib import Path
import hashlib
import struct
import sys
from fat32_directory_growth_image_check import geometry

CLEAN = 0x08000000
NO_ERROR = 0x04000000
MODES = {"dirty": CLEAN, "hard-error": NO_ERROR, "status-mismatch": CLEAN}


def seed(image: Path, snapshot: Path, mode: str) -> None:
    if mode not in MODES:
        raise ValueError("unsupported recovery fixture mode")
    data = bytearray(image.read_bytes())
    _, reserved, copies, fat_size, _, _, _ = geometry(data)
    for copy in range(copies):
        offset = (reserved + copy * fat_size) * 512 + 4
        value = struct.unpack_from("<I", data, offset)[0]
        if value & (CLEAN | NO_ERROR) != CLEAN | NO_ERROR:
            raise ValueError("recovery seed must begin with clean status in every FAT")
    for copy in range(copies):
        if mode == "status-mismatch" and copy == 0:
            continue
        offset = (reserved + copy * fat_size) * 512 + 4
        value = struct.unpack_from("<I", data, offset)[0]
        struct.pack_into("<I", data, offset, value & ~MODES[mode])
    image.write_bytes(data)
    snapshot.write_text(hashlib.sha256(data).hexdigest() + "\n", encoding="ascii")
    print(f"FAT32 recovery fixture OK: {mode}")


def verify(image: Path, snapshot: Path) -> None:
    expected = snapshot.read_text(encoding="ascii").strip()
    if len(expected) != 64 or any(c not in "0123456789abcdef" for c in expected):
        raise ValueError("invalid recovery image baseline")
    actual = hashlib.sha256(image.read_bytes()).hexdigest()
    if actual != expected:
        raise ValueError("recovery boot changed the disk image or cleared status evidence")
    print("FAT32 recovery image proof OK: entire disk unchanged after read-only boot")


if __name__ == "__main__":
    if len(sys.argv) not in (4, 5) or sys.argv[1] not in ("seed", "verify"):
        raise SystemExit("usage: fat32_recovery_image_check.py seed|verify image snapshot [mode]")
    try:
        if sys.argv[1] == "seed":
            if len(sys.argv) != 5:
                raise ValueError("seed requires dirty, hard-error or status-mismatch mode")
            seed(Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4])
        else:
            if len(sys.argv) != 4:
                raise ValueError("verify does not accept a mode")
            verify(Path(sys.argv[2]), Path(sys.argv[3]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
