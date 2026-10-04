#!/usr/bin/env python3
"""Fill the CI root cluster, then prove its extension from the reopened image."""
from pathlib import Path
import struct
import sys


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def geometry(data: bytes) -> tuple[int, int, int, int, int, int, int]:
    if len(data) < 512 or data[510:512] != b"\x55\xaa" or u16(data, 11) != 512:
        raise ValueError("invalid CI FAT32 boot sector")
    spc, reserved, copies, fat_size = data[13], u16(data, 14), data[16], u32(data, 36)
    root, volume = u32(data, 44), u32(data, 32)
    first_data = reserved + copies * fat_size
    if (spc != 1 or reserved == 0 or copies != 2 or fat_size == 0 or root < 2 or
            u16(data, 40) & 0x80 or first_data >= volume or volume * 512 > len(data)):
        raise ValueError("unexpected CI FAT32 geometry")
    return spc, reserved, copies, fat_size, root, volume, first_data


def cluster_offset(cluster: int, geo: tuple[int, ...]) -> int:
    spc, _, _, _, _, volume, first_data = geo
    lba = first_data + (cluster - 2) * spc
    if cluster < 2 or lba < first_data or lba + spc > volume:
        raise ValueError("directory cluster outside volume")
    return lba * 512


def fat_value(data: bytes, cluster: int, copy: int, geo: tuple[int, ...]) -> int:
    _, reserved, _, fat_size, _, _, _ = geo
    if cluster * 4 + 4 > fat_size * 512:
        raise ValueError("cluster outside FAT")
    return u32(data, (reserved + copy * fat_size) * 512 + cluster * 4) & 0x0FFFFFFF


def seed(image: Path, snapshot: Path) -> None:
    data = bytearray(image.read_bytes())
    geo = geometry(data)
    root = geo[4]
    if any(fat_value(data, root, copy, geo) < 0x0FFFFFF8 for copy in range(2)):
        raise ValueError("CI seed root must be a single cluster")
    offset = cluster_offset(root, geo)
    raw = bytearray(data[offset:offset + 512])
    if raw[:11] != b"RUNTIME TXT" or raw[32:43] != b"GUARD   TXT" or raw[64] != 0:
        raise ValueError("CI seed files do not match the expected fixture")
    for slot in range(2, 16):
        entry = bytearray(32)
        entry[:11] = f"F{slot:07d}TMP".encode("ascii")
        entry[11] = 0x20
        raw[slot * 32:(slot + 1) * 32] = entry
    data[offset:offset + 512] = raw
    image.write_bytes(data)
    snapshot.write_bytes(raw)
    print("FAT32 directory growth fixture OK: original root cluster is full")


def verify(image: Path, snapshot: Path) -> None:
    data = image.read_bytes()
    geo = geometry(data)
    root = geo[4]
    original = snapshot.read_bytes()
    if len(original) != 512 or original[:11] != b"RUNTIME TXT" or original[32:43] != b"GUARD   TXT":
        raise ValueError("invalid root-directory baseline")
    expected = bytearray(original)
    runtime_size = len(b"SUIRABOX-WRITTEN-FAT32\n" + b"SUIRABOX-APPEND\n" * 48)
    struct.pack_into("<I", expected, 28, runtime_size)
    offset = cluster_offset(root, geo)
    if data[offset:offset + 512] != expected:
        raise ValueError("an existing root entry changed outside the expected runtime size")
    next_cluster = fat_value(data, root, 0, geo)
    if next_cluster < 2 or next_cluster == root or next_cluster >= 0x0FFFFFF8:
        raise ValueError("root directory did not grow")
    if fat_value(data, root, 1, geo) != next_cluster:
        raise ValueError("root extension differs between FAT copies")
    if any(fat_value(data, next_cluster, copy, geo) < 0x0FFFFFF8 for copy in range(2)):
        raise ValueError("new directory cluster is not terminated in both FAT copies")
    new_offset = cluster_offset(next_cluster, geo)
    added = data[new_offset:new_offset + 512]
    if added[:11] != b"NEWFILE TXT" or added[11] != 0x20 or u32(added, 28) != len(b"SUIRABOX-CREATED-FAT32\n"):
        raise ValueError("created file missing from appended directory cluster")
    if any(added[32:]):
        raise ValueError("new directory end marker/slack is not zeroed")
    print("FAT32 directory growth image proof OK: mirrored extension and existing entries preserved")


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[1] not in ("seed", "verify"):
        raise SystemExit("usage: fat32_directory_growth_image_check.py seed|verify image snapshot")
    try:
        (seed if sys.argv[1] == "seed" else verify)(Path(sys.argv[2]), Path(sys.argv[3]))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(str(error)) from error
