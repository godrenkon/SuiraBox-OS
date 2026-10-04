import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest

from fat32_directory_growth_image_check import seed
from fat32_mkdir_image_check import PAYLOAD, raw_entry, verify_mkdir


class MkdirImageCheckTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.image = Path(self.tmp.name) / "disk.img"
        self.snapshot = Path(self.tmp.name) / "root.bin"
        data = bytearray(64 * 512)
        struct.pack_into("<H", data, 11, 512)
        data[13], data[16] = 1, 2
        struct.pack_into("<H", data, 14, 1)
        for offset, value in ((32, 64), (36, 1), (44, 2)):
            struct.pack_into("<I", data, offset, value)
        data[510:512] = b"\x55\xaa"
        for copy in range(2):
            for c in range(11):
                struct.pack_into("<I", data, (1 + copy) * 512 + c * 4, 0x0FFFFFFF)
        data[3 * 512:3 * 512 + 32] = raw_entry(b"RUNTIME TXT", 0x20, 3, 23)
        data[3 * 512 + 32:3 * 512 + 64] = raw_entry(b"GUARD   TXT", 0x20, 6, 23)
        self.image.write_bytes(data)
        with contextlib.redirect_stdout(io.StringIO()):
            seed(self.image, self.snapshot)
        data = bytearray(self.image.read_bytes())
        for copy in range(2):
            struct.pack_into("<I", data, (1 + copy) * 512 + 2 * 4, 5)
            struct.pack_into("<I", data, (1 + copy) * 512 + 3 * 4, 4)
        struct.pack_into("<I", data, 3 * 512 + 28, 791)
        data[6 * 512:6 * 512 + 32] = raw_entry(b"NEWFILE TXT", 0x20, 7, 23)
        data[6 * 512 + 32:6 * 512 + 64] = raw_entry(b"SAVES      ", 0x10, 8)
        for sector, child, parent, entry in (
            (9, 8, 0, raw_entry(b"WORLDS     ", 0x10, 9)),
            (10, 9, 8, raw_entry(b"LEVEL   DAT", 0x20, 10, len(PAYLOAD))),
        ):
            data[sector * 512:sector * 512 + 32] = raw_entry(b".          ", 0x10, child)
            data[sector * 512 + 32:sector * 512 + 64] = raw_entry(b"..         ", 0x10, parent)
            data[sector * 512 + 64:sector * 512 + 96] = entry
        data[11 * 512:11 * 512 + len(PAYLOAD)] = PAYLOAD
        self.valid = bytes(data)
        self.image.write_bytes(self.valid)

    def test_valid_nested_directory_and_file(self):
        with contextlib.redirect_stdout(io.StringIO()):
            verify_mkdir(self.image, self.snapshot)

    def test_bad_dot_parent_and_slack_rejected(self):
        for offset in (9 * 512 + 26, 9 * 512 + 58, 10 * 512 + 58, 10 * 512 + 96):
            with self.subTest(offset=offset):
                data = bytearray(self.valid)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    verify_mkdir(self.image, self.snapshot)

    def test_allocation_mismatch_and_alias_rejected(self):
        for offset, value, fmt in ((2 * 512 + 8 * 4, 0, "<I"), (6 * 512 + 32 + 26, 7, "<H")):
            with self.subTest(offset=offset):
                data = bytearray(self.valid)
                struct.pack_into(fmt, data, offset, value)
                self.image.write_bytes(data)
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    verify_mkdir(self.image, self.snapshot)

    def test_payload_and_size_corruption_rejected(self):
        for offset in (11 * 512, 10 * 512 + 64 + 28):
            with self.subTest(offset=offset):
                data = bytearray(self.valid)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    verify_mkdir(self.image, self.snapshot)


if __name__ == "__main__":
    unittest.main()
