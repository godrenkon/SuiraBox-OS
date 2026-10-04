import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from fat32_dirty_image_check import prepare, verify
from fat32_recovery_image_check import CLEAN


class DirtyImageCheckTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.image = Path(self.tmp.name) / "disk.img"
        self.snapshot = Path(self.tmp.name) / "expected.sha256"
        data = bytearray(64 * 512)
        struct.pack_into("<H", data, 11, 512)
        data[13], data[16] = 1, 2
        struct.pack_into("<H", data, 14, 1)
        for off, value in ((32, 64), (36, 1), (44, 2)):
            struct.pack_into("<I", data, off, value)
        data[510:512] = b"\x55\xaa"
        for copy in range(2):
            struct.pack_into("<I", data, (copy + 1) * 512 + 4, (0xA0000000 if copy == 0 else 0xB0000000) | 0x0FFFFFFF)
        self.original = bytes(data)
        self.image.write_bytes(data)
        with contextlib.redirect_stdout(io.StringIO()):
            prepare(self.image, self.snapshot)
        self.assertEqual(self.image.read_bytes(), self.original)
        for copy in range(2):
            offset = (copy + 1) * 512 + 4
            value = struct.unpack_from("<I", data, offset)[0]
            struct.pack_into("<I", data, offset, value & ~CLEAN)
        self.dirty = bytes(data)

    def test_durable_marker_only_passes(self):
        self.image.write_bytes(self.dirty)
        with contextlib.redirect_stdout(io.StringIO()):
            verify(self.image, self.snapshot)

    def test_clean_or_partial_marker_rejected(self):
        for data in (self.original, self.original[:512 + 8] + self.dirty[512 + 8:]):
            with self.subTest(data=data[:8]):
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.snapshot)

    def test_published_metadata_or_modified_data_rejected(self):
        for offset in (3 * 512, 4 * 512, len(self.dirty) - 1):
            with self.subTest(offset=offset):
                data = bytearray(self.dirty)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.snapshot)

    def test_dirty_seed_rejected_without_mutation(self):
        self.image.write_bytes(self.dirty)
        with self.assertRaises(ValueError):
            prepare(self.image, self.snapshot)
        self.assertEqual(self.image.read_bytes(), self.dirty)


if __name__ == "__main__":
    unittest.main()
