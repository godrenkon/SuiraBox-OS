#!/usr/bin/env python3
"""Compare the complete rename/reboot fixture against its untouched seed."""
from pathlib import Path
import struct
import sys
from fat32_directory_growth_image_check import geometry, cluster_offset, fat_value, u16, u32
from fat32_mkdir_image_check import first_cluster, raw_entry
from fat32_recovery_image_check import CLEAN, NO_ERROR

PAYLOAD = b"SUIRABOX RENAMED\n"


def verify(image: Path, seed: Path) -> None:
    actual, original = image.read_bytes(), seed.read_bytes()
    geo = geometry(original)
    if geometry(actual) != geo or len(actual) != len(original):
        raise ValueError("rename geometry differs from seed")
    _, reserved, copies, fat_size, root, _, _ = geo
    root_offset = cluster_offset(root, geo)
    if (original[root_offset:root_offset + 11] != b"RUNTIME TXT" or original[root_offset + 32] != 0 or
            any(fat_value(original, root, copy, geo) < 0x0FFFFFF8 for copy in range(copies))):
        raise ValueError("unexpected rename seed directory")
    for copy in range(copies):
        if u32(original, (reserved + copy * fat_size) * 512 + 4) & (CLEAN | NO_ERROR) != CLEAN | NO_ERROR:
            raise ValueError("rename seed status must be clean")
    world = first_cluster(actual[root_offset + 32:root_offset + 64])
    saves = first_cluster(actual[root_offset + 64:root_offset + 96])
    saves_offset = cluster_offset(saves, geo)
    nested = first_cluster(actual[saves_offset + 64:saves_offset + 96])
    allocations = (world, saves, nested)
    if len(set(allocations)) != 3:
        raise ValueError("renamed files and directory alias one another")
    for cluster in allocations:
        cluster_offset(cluster, geo)
        if any(fat_value(original, cluster, copy, geo) != 0 for copy in range(copies)):
            raise ValueError("renamed file or directory aliases an existing allocation")
    expected = bytearray(original)
    expected[root_offset + 32:root_offset + 64] = raw_entry(b"WORLD   DAT", 0x20, world, len(PAYLOAD) + 1)
    expected[root_offset + 64:root_offset + 96] = raw_entry(b"SAVES      ", 0x10, saves)
    directory = (raw_entry(b".          ", 0x10, saves) + raw_entry(b"..         ", 0x10, 0) +
                 raw_entry(b"WORLD   DAT", 0x20, nested, len(PAYLOAD)) + bytes(512 - 96))
    expected[saves_offset:saves_offset + 512] = directory
    for cluster, payload in ((world, PAYLOAD + b"!"), (nested, PAYLOAD)):
        offset = cluster_offset(cluster, geo)
        expected[offset:offset + 512] = payload + bytes(512 - len(payload))
    for cluster in allocations:
        for copy in range(copies):
            offset = (reserved + copy * fat_size) * 512 + cluster * 4
            struct.pack_into("<I", expected, offset, u32(original, offset) & 0xF0000000 | 0x0FFFFFFF)
    fsinfo = u16(original, 48)
    offset = fsinfo * 512
    if (0 < fsinfo < reserved and u32(original, offset) == 0x41615252 and
            u32(original, offset + 484) == 0x61417272 and u32(original, offset + 508) == 0xAA550000):
        struct.pack_into("<II", expected, offset + 488, 0xFFFFFFFF, 0xFFFFFFFF)
    if actual != expected:
        raise ValueError("rename changed unexpected bytes, retained a staged name, or lost data/clean status")
    print("FAT32 rename image proof OK: exact root/nested names, held-handle append, clean statuses and seed retained")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: fat32_rename_image_check.py image seed")
    try:
        verify(Path(sys.argv[1]), Path(sys.argv[2]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
