#!/usr/bin/env python3
"""Verify every RGB pixel of the screen captured from the actual QEMU display."""
from pathlib import Path
import re
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


def font_rows() -> list[tuple[int, ...]]:
    source = (Path(__file__).resolve().parent.parent / "userspace/font_bitmap.h").read_text()
    rows = [tuple(map(int, match.split(","))) for match in re.findall(r"\{([0-9, ]+)\}", source)]
    if len(rows) != 95 or any(len(row) != 7 or any(value > 31 for value in row) for row in rows):
        raise ValueError("invalid built-in ASCII font asset")
    return rows


def text_labels(width: int, height: int):
    return [
        (24,16,"SuiraBox OS",0xe6eef2,0x152536,2),
        (24,96,"Home\nFiles\nSettings",0xe6eef2,0x18394b,2),
        (200,82,"Graphics and text",0xe6eef2,0x167ca4,2),
        (208,288,"ASCII: AaZz 0123456789",0x152536,0xe6eef2,2),
        (208,312,"UTF-8: 日本語 / é / 🎮",0x152536,0xe6eef2,2),
        (208,344,"A\tB\rC\nNext line",0x152536,0xe6eef2,1),
        (24,height-22,"Display + text ready",0xe6eef2,0x152536,1),
        (width-120,height-22,"Continue",0x0c1018,0x259b72,1),
        (-3,54,"Clip",0xf2ae43,0x0c1018,1),
        (width-5,52,"Right",0x259b72,0x0c1018,2),
        (0,height-5,"Bottom",0xf2ae43,0x152536,2),
    ]


def paint_text(pixels: bytearray, width: int, height: int, labels) -> None:
    font = font_rows()
    for origin_x, origin_y, text, foreground, background, scale in labels:
        column = line = 0
        for character in text:
            if character == "\n":
                column = 0
                line += 1
                continue
            if character == "\r":
                column = 0
                continue
            if character == "\t":
                column += 4 - column % 4
                continue
            code = ord(character) if 32 <= ord(character) <= 126 else ord("?")
            # Decode the bitmap to an 8x8 cell independently of the C renderer,
            # then replicate each pixel and clip each output coordinate.
            for row in range(8):
                for cell_x in range(8):
                    ink = row < 7 and 1 <= cell_x <= 5 and font[code-32][row] & (1 << (5-cell_x))
                    color = (foreground if ink else background).to_bytes(3, "big")
                    for dy in range(scale):
                        for dx in range(scale):
                            x = origin_x + column * 8 * scale + cell_x * scale + dx
                            y = origin_y + line * 8 * scale + row * scale + dy
                            if 0 <= x < width and 0 <= y < height:
                                offset = (y * width + x) * 3
                                pixels[offset:offset+3] = color
            column += 1


def expected_pixels(width: int, height: int, text_proof: bool = False, boot_text: bool = False, input_proof: bool = False) -> bytes:
    if width < 640 or height < 480:
        raise ValueError("display proof requires at least 640x480")
    pixels = bytearray(bytes((12, 16, 24)) * width * height)
    if boot_text or input_proof:
        labels = [
            (24,16,"SuiraBox OS",0xe6eef2,0x0c1018,2),
            (24,48,"Starting userspace services...",0xe6eef2,0x0c1018,2),
            (24,80,"Keyboard ready: type, Backspace, Enter",0xe6eef2,0x0c1018,2),
            (24,112,"> " + " " * 32,0xe6eef2,0x0c1018,2),
        ]
        if input_proof:
            labels.append((24,144,"Last submitted: AC" + " " * 30,0x259b72,0x0c1018,2))
        paint_text(pixels, width, height, labels)
        return bytes(pixels)
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
    if text_proof:
        paint_text(pixels, width, height, text_labels(width, height))
    return bytes(pixels)


def verify_bytes(data: bytes, text_proof: bool = False, boot_text: bool = False, input_proof: bool = False) -> None:
    width, height, pixels = read_ppm(data)
    expected = expected_pixels(width, height, text_proof, boot_text, input_proof)
    if pixels != expected:
        first = next(index for index, pair in enumerate(zip(pixels, expected)) if pair[0] != pair[1]) // 3
        raise ValueError(f"display mismatch at ({first % width},{first // width}): "
                         f"actual {tuple(pixels[first * 3:first * 3 + 3])}, expected {tuple(expected[first * 3:first * 3 + 3])}")
    kind = "keyboard-edit" if input_proof else "boot-text" if boot_text else "bitmap-text" if text_proof else "surface"
    print(f"QEMU display image proof OK: exact {width}x{height} RGB {kind}, clipping boundaries and rejected-request pixels")


if __name__ == "__main__":
    try:
        verify_bytes(Path(sys.argv[1]).read_bytes(), "--text-proof" in sys.argv[2:], "--boot-text" in sys.argv[2:], "--input-proof" in sys.argv[2:])
    except (OSError, ValueError, IndexError) as error:
        raise SystemExit(str(error)) from error
