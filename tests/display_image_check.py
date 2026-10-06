#!/usr/bin/env python3
"""Verify every RGB pixel of the screen captured from the actual QEMU display."""
from pathlib import Path
import sys


def read_ppm(data: bytes) -> tuple[int, int, bytes]:
    position = 0
    tokens = []
    for _ in range(4):
        while position < len(data):
            if data[position] in b" \t\r\n":
                position += 1
            elif data[position] == ord("#"):
                end = data.find(b"\n", position)
                if end < 0:
                    raise ValueError("unterminated PPM comment")
                position = end + 1
            else:
                break
        start = position
        while position < len(data) and data[position] not in b" \t\r\n":
            position += 1
        tokens.append(data[start:position])
    if tokens[0] != b"P6" or tokens[3] != b"255":
        raise ValueError("expected 8-bit binary RGB PPM")
    width, height = int(tokens[1]), int(tokens[2])
    if not (0 < width <= 4096 and 0 < height <= 4096) or position >= len(data):
        raise ValueError("invalid display dimensions or missing PPM separator")
    # Consume only the header separator; a first pixel may itself be whitespace.
    position += 2 if data[position:position + 2] == b"\r\n" else 1
    pixels = data[position:]
    if len(pixels) != width * height * 3:
        raise ValueError("PPM pixel payload size mismatch")
    return width, height, pixels


def expected_pixels(width: int, height: int) -> bytes:
    if width < 640 or height < 480:
        raise ValueError("display proof requires at least 640x480")
    pixels = bytearray(bytes((12, 16, 24)) * width * height)
    def fill(x, y, w, h, color):
        row = bytes(color) * w
        for index in range(y, y + h):
            offset = (index * width + x) * 3
            pixels[offset:offset + w * 3] = row
    fill(0, 0, width, 48, (21, 37, 54))
    fill(16, 72, 144, height - 112, (24, 57, 75))
    fill(184, 72, width - 208, height - 112, (230, 238, 242))
    fill(184, 72, width - 208, 36, (22, 124, 164))
    fill(208, 136, 128, 128, (37, 155, 114))
    fill(360, 136, 128, 128, (242, 174, 67))
    fill(512, 136, 104, 128, (82, 124, 189))
    fill(0, height - 28, width, 28, (21, 37, 54))
    fill(width - 120, height - 22, 104, 16, (37, 155, 114))
    return bytes(pixels)


def verify_bytes(data: bytes) -> None:
    width, height, pixels = read_ppm(data)
    expected = expected_pixels(width, height)
    if pixels != expected:
        first = next(index for index, pair in enumerate(zip(pixels, expected)) if pair[0] != pair[1]) // 3
        raise ValueError(f"display mismatch at ({first % width},{first // width}): "
                         f"actual {tuple(pixels[first * 3:first * 3 + 3])}, expected {tuple(expected[first * 3:first * 3 + 3])}")
    print(f"QEMU display image proof OK: exact {width}x{height} RGB surface, clipping boundaries and rejected-request pixels")


if __name__ == "__main__":
    try:
        verify_bytes(Path(sys.argv[1]).read_bytes())
    except (OSError, ValueError, IndexError) as error:
        raise SystemExit(str(error)) from error
