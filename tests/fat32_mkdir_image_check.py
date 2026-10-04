#!/usr/bin/env python3
"""Check mkdir dot entries, mirrored allocation and nested data after QEMU exit."""
from pathlib import Path
import struct
import sys
from fat32_directory_growth_image_check import geometry, cluster_offset, fat_value, u16, u32, verify

PAYLOAD = b"SUIRABOX-WORLD-DATA\n"


def first_cluster(entry: bytes) -> int:
    return u16(entry, 20) << 16 | u16(entry, 26)


def raw_entry(name: bytes, attributes: int, cluster: int, size: int = 0) -> bytes:
    entry = bytearray(32)
    entry[:11] = name
    entry[11] = attributes
    struct.pack_into("<H", entry, 20, cluster >> 16)
    struct.pack_into("<H", entry, 26, cluster & 0xFFFF)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


def verify_mkdir(image: Path, snapshot: Path) -> None:
    verify(image, snapshot, mkdir=True)
    data = image.read_bytes()
    geo = geometry(data)
    root = geo[4]
    root_bytes = data[cluster_offset(root, geo):cluster_offset(root, geo) + 512]
    extension = fat_value(data, root, 0, geo)
    added = data[cluster_offset(extension, geo):cluster_offset(extension, geo) + 512]
    saves = first_cluster(added[32:64])
    saves_bytes = data[cluster_offset(saves, geo):cluster_offset(saves, geo) + 512]
    worlds = first_cluster(saves_bytes[64:96])
    worlds_bytes = data[cluster_offset(worlds, geo):cluster_offset(worlds, geo) + 512]
    level = first_cluster(worlds_bytes[64:96])
    clusters = [root, extension, saves, worlds, level, first_cluster(root_bytes[:32]),
                first_cluster(root_bytes[32:64]), first_cluster(added[:32])]
    # Include the runtime file's second cluster in overlap detection too.
    clusters.append(fat_value(data, clusters[5], 0, geo))
    if len(set(clusters)) != len(clusters):
        raise ValueError("created directories/files alias another allocated cluster")
    for cluster in (saves, worlds, level):
        cluster_offset(cluster, geo)
        if any(fat_value(data, cluster, copy, geo) < 0x0FFFFFF8 for copy in range(2)):
            raise ValueError("created directory/file lacks mirrored EOC")
    if added[32:64] != raw_entry(b"SAVES      ", 0x10, saves):
        raise ValueError("invalid parent SAVES entry")
    for raw, child, parent, name, target, attr, size in (
        (saves_bytes, saves, 0, b"WORLDS     ", worlds, 0x10, 0),
        (worlds_bytes, worlds, saves, b"LEVEL   DAT", level, 0x20, len(PAYLOAD)),
    ):
        if (raw[:32] != raw_entry(b".          ", 0x10, child) or
                raw[32:64] != raw_entry(b"..         ", 0x10, parent) or
                raw[64:96] != raw_entry(name, attr, target, size) or any(raw[96:])):
            raise ValueError("directory dot entries, nested entry or zeroed slack invalid")
    offset = cluster_offset(level, geo)
    if data[offset:offset + len(PAYLOAD)] != PAYLOAD or any(data[offset + len(PAYLOAD):offset + 512]):
        raise ValueError("nested world file content or zeroed allocation slack invalid")
    print("FAT32 mkdir image proof OK: dot entries, mirrored directories and nested file persisted")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: fat32_mkdir_image_check.py image snapshot")
    try:
        verify_mkdir(Path(sys.argv[1]), Path(sys.argv[2]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
