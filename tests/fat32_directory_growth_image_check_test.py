from pathlib import Path
import struct
import tempfile
import unittest

from fat32_directory_growth_image_check import seed, verify


class DirectoryGrowthImageTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.image = Path(self.temp.name) / "fat32.img"
        self.snapshot = Path(self.temp.name) / "root.bin"
        data = bytearray(64 * 512)
        struct.pack_into("<H", data, 11, 512)
        data[13] = 1
        struct.pack_into("<H", data, 14, 1)
        data[16] = 2
        struct.pack_into("<III", data, 32, 64, 1, 0)
        struct.pack_into("<I", data, 44, 2)
        data[510:512] = b"\x55\xaa"
        for fat in (512, 1024):
            struct.pack_into("<I", data, fat + 8, 0x0FFFFFFF)
        for slot, name, cluster in ((0, b"RUNTIME TXT", 3), (1, b"GUARD   TXT", 4)):
            offset = 1536 + slot * 32
            data[offset:offset + 11] = name
            data[offset + 11] = 0x20
            struct.pack_into("<H", data, offset + 26, cluster)
            struct.pack_into("<I", data, offset + 28, 23)
        self.image.write_bytes(data)
        seed(self.image, self.snapshot)

    def grown(self):
        data = bytearray(self.image.read_bytes())
        struct.pack_into("<I", data, 1536 + 28, 791)
        for fat in (512, 1024):
            struct.pack_into("<I", data, fat + 8, 5)
            struct.pack_into("<I", data, fat + 20, 0x0FFFFFFF)
        data[3072:3083] = b"NEWFILE TXT"
        data[3083] = 0x20
        struct.pack_into("<H", data, 3072 + 26, 6)
        struct.pack_into("<I", data, 3072 + 28, 23)
        return data

    def test_growth_and_preserved_entries_pass(self):
        self.image.write_bytes(self.grown())
        verify(self.image, self.snapshot)

    def test_unchanged_directory_is_not_a_proof(self):
        data = bytearray(self.image.read_bytes())
        struct.pack_into("<I", data, 1536 + 28, 791)
        self.image.write_bytes(data)
        with self.assertRaisesRegex(ValueError, "did not grow"):
            verify(self.image, self.snapshot)

    def test_directory_mirror_and_slack_corruption_fail(self):
        for offset in (1536 + 32 + 11, 1024 + 8, 3072 + 32):
            with self.subTest(offset=offset):
                data = self.grown()
                data[offset] ^= 1
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.snapshot)

    def test_unexpected_seed_rejected_without_mutation(self):
        data = self.grown()
        self.image.write_bytes(data)
        with self.assertRaises(ValueError):
            seed(self.image, self.snapshot)
        self.assertEqual(self.image.read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
