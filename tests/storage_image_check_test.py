import struct
import tempfile
import unittest
from pathlib import Path

from storage_image_check import COMMIT, SEED, SECTOR_SIZE, VOLUME_SECTORS, seed, verify


class StorageImageCheckTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "fixture.img"
        boot = bytearray(SECTOR_SIZE)
        struct.pack_into("<H", boot, 11, SECTOR_SIZE)
        struct.pack_into("<I", boot, 32, VOLUME_SECTORS)
        boot[510:512] = b"\x55\xaa"
        with self.path.open("wb") as image:
            image.write(boot)
            image.truncate(VOLUME_SECTORS * SECTOR_SIZE)

    def test_seed_requires_exact_volume(self):
        with self.path.open("r+b") as image:
            image.truncate(VOLUME_SECTORS * SECTOR_SIZE - 1)
        with self.assertRaises(ValueError):
            seed(self.path)

    def test_seed_rejects_bad_bpb_without_appending(self):
        with self.path.open("r+b") as image:
            image.seek(32)
            image.write(struct.pack("<I", VOLUME_SECTORS + 1))
        with self.assertRaises(ValueError):
            seed(self.path)
        self.assertEqual(self.path.stat().st_size, VOLUME_SECTORS * SECTOR_SIZE)

    def test_unwritten_and_partial_commit_rejected(self):
        seed(self.path)
        with self.path.open("rb") as image:
            image.seek(VOLUME_SECTORS * SECTOR_SIZE)
            self.assertEqual(image.read(), SEED)
        with self.assertRaises(ValueError):
            verify(self.path)
        with self.path.open("r+b") as image:
            image.seek(VOLUME_SECTORS * SECTOR_SIZE)
            image.write(COMMIT)
            image.seek(-1, 2)
            image.write(b"\x01")
        with self.assertRaises(ValueError):
            verify(self.path)

    def test_full_commit_survives_reopen(self):
        seed(self.path)
        with self.path.open("r+b") as image:
            image.seek(VOLUME_SECTORS * SECTOR_SIZE)
            image.write(COMMIT)
        verify(self.path)


if __name__ == "__main__":
    unittest.main()
