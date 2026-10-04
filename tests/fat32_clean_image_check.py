#!/usr/bin/env python3
"""Prove clean unmount persisted exactly one previously unsynced CI file."""
from pathlib import Path
import struct
import sys
from fat32_directory_growth_image_check import geometry, cluster_offset, fat_value, u16, u32
from fat32_mkdir_image_check import first_cluster, raw_entry
from fat32_recovery_image_check import CLEAN, NO_ERROR

PAYLOAD = b"SUIRABOX CLEAN\n"


def verify(image: Path, seed: Path) -> None:
    actual, original = image.read_bytes(), seed.read_bytes()
    geo = geometry(original)
    if geometry(actual) != geo or len(actual) != len(original):
        raise ValueError("clean-unmount geometry differs from seed")
    _, reserved, copies, fat_size, root, _, _ = geo
    root_offset = cluster_offset(root, geo)
    if (original[root_offset:root_offset + 11] != b"RUNTIME TXT" or
            original[root_offset + 32] != 0 or
            any(fat_value(original, root, copy, geo) < 0x0FFFFFF8 for copy in range(copies))):
        raise ValueError("unexpected clean-unmount seed directory")
    for copy in range(copies):
        if u32(original, (reserved + copy * fat_size) * 512 + 4) & (CLEAN | NO_ERROR) != CLEAN | NO_ERROR:
            raise ValueError("clean-unmount seed status must be clean")
    entry = actual[root_offset + 32:root_offset + 64]
    cluster = first_cluster(entry)
    data_offset = cluster_offset(cluster, geo)
    if any(fat_value(original, cluster, copy, geo) != 0 for copy in range(copies)):
        raise ValueError("clean file aliases an existing allocation")
    expected = bytearray(original)
    expected[root_offset + 32:root_offset + 64] = raw_entry(b"CLEAN   TXT", 0x20, cluster, len(PAYLOAD))
    expected[data_offset:data_offset + 512] = PAYLOAD + bytes(512 - len(PAYLOAD))
    for copy in range(copies):
        offset = (reserved + copy * fat_size) * 512 + cluster * 4
        struct.pack_into("<I", expected, offset, u32(original, offset) & 0xF0000000 | 0x0FFFFFFF)
    fsinfo = u16(original, 48)
    offset = fsinfo * 512
    if (0 < fsinfo < reserved and u32(original, offset) == 0x41615252 and
            u32(original, offset + 484) == 0x61417272 and u32(original, offset + 508) == 0xAA550000):
        struct.pack_into("<II", expected, offset + 488, 0xFFFFFFFF, 0xFFFFFFFF)
    if actual != expected:
        raise ValueError("clean unmount did not persist exactly the new file, mirrored FAT and clean status")
    print("FAT32 clean image proof OK: unsynced file persisted, clean statuses and seed data preserved")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: fat32_clean_image_check.py image seed")
    try:
        verify(Path(sys.argv[1]), Path(sys.argv[2]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
