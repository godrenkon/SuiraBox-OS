import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from fat32_recovery_image_check import CLEAN, NO_ERROR, seed, verify


class RecoveryImageCheckTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.image = Path(self.tmp.name) / "disk.img"
        self.snapshot = Path(self.tmp.name) / "disk.sha256"
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
        self.image.write_bytes(self.original)

    def test_modes_change_only_expected_status_bits(self):
        for mode in ("dirty", "hard-error", "status-mismatch"):
            with self.subTest(mode=mode), contextlib.redirect_stdout(io.StringIO()):
                self.image.write_bytes(self.original)
                seed(self.image, self.snapshot, mode)
                data = self.image.read_bytes()
                expected = bytearray(self.original)
                for copy in range(2):
                    if mode == "status-mismatch" and copy == 0:
                        continue
                    offset = (copy + 1) * 512 + 4
                    value = struct.unpack_from("<I", expected, offset)[0]
                    struct.pack_into("<I", expected, offset, value & ~(NO_ERROR if mode == "hard-error" else CLEAN))
                self.assertEqual(data, expected)
                verify(self.image, self.snapshot)

    def test_mutation_or_cleared_evidence_rejected(self):
        with contextlib.redirect_stdout(io.StringIO()):
            seed(self.image, self.snapshot, "dirty")
        seeded = self.image.read_bytes()
        for offset in (0, 512 + 7, 4096, len(seeded) - 1):
            with self.subTest(offset=offset):
                data = bytearray(seeded)
                data[offset] ^= 0x08
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.snapshot)

    def test_invalid_baseline_rejected(self):
        for text in ("", "x" * 64, "0" * 63):
            with self.subTest(text=text):
                self.snapshot.write_text(text, encoding="ascii")
                with self.assertRaises(ValueError):
                    verify(self.image, self.snapshot)

    def test_bad_seed_rejected_without_mutation(self):
        for mode, dirty in (("unsupported", False), ("dirty", True)):
            with self.subTest(mode=mode):
                data = bytearray(self.original)
                if dirty:
                    struct.pack_into("<I", data, 512 + 4, 0x07FFFFFF)
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    seed(self.image, self.snapshot, mode)
                self.assertEqual(self.image.read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
