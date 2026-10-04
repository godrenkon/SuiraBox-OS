import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from fat32_clean_image_check import PAYLOAD, verify
from fat32_mkdir_image_check import raw_entry
from fat32_recovery_image_check import CLEAN, NO_ERROR


class CleanImageCheckTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.seed, self.image = Path(tmp.name) / "seed.img", Path(tmp.name) / "image.img"
        data = bytearray(64 * 512)
        struct.pack_into("<H", data, 11, 512)
        data[13], data[16] = 1, 2
        struct.pack_into("<H", data, 14, 1)
        for offset, value in ((32, 64), (36, 1), (44, 2)):
            struct.pack_into("<I", data, offset, value)
        data[510:512] = b"\x55\xaa"
        for copy in range(2):
            for cluster in range(4):
                struct.pack_into("<I", data, (copy + 1) * 512 + cluster * 4,
                                 (0xA0000000 if copy == 0 else 0xB0000000) | 0x0FFFFFFF)
        data[3 * 512:3 * 512 + 32] = raw_entry(b"RUNTIME TXT", 0x20, 3, 5)
        data[4 * 512:4 * 512 + 5] = b"hello"
        self.seed.write_bytes(data)
        data[3 * 512 + 32:3 * 512 + 64] = raw_entry(b"CLEAN   TXT", 0x20, 4, len(PAYLOAD))
        data[5 * 512:5 * 512 + len(PAYLOAD)] = PAYLOAD
        for copy in range(2):
            struct.pack_into("<I", data, (copy + 1) * 512 + 16, 0x0FFFFFFF)
        self.good = bytes(data)
        self.image.write_bytes(data)

    def test_exact_clean_file_passes(self):
        with contextlib.redirect_stdout(io.StringIO()):
            verify(self.image, self.seed)

    def test_dirty_hard_error_or_reserved_status_change_rejected(self):
        for mask in (CLEAN, NO_ERROR, 0x10000000):
            for copy in range(2):
                with self.subTest(mask=mask, copy=copy):
                    data = bytearray(self.good)
                    offset = (copy + 1) * 512 + 4
                    struct.pack_into("<I", data, offset, struct.unpack_from("<I", data, offset)[0] ^ mask)
                    self.image.write_bytes(data)
                    with self.assertRaises(ValueError):
                        verify(self.image, self.seed)

    def test_metadata_fat_payload_or_existing_bytes_changed_rejected(self):
        for offset in (3 * 512 + 60, 1024 + 16, 5 * 512, 4 * 512, len(self.good) - 1):
            with self.subTest(offset=offset):
                data = bytearray(self.good)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.seed)

    def test_existing_cluster_alias_rejected(self):
        data = bytearray(self.good)
        struct.pack_into("<H", data, 3 * 512 + 32 + 26, 3)
        self.image.write_bytes(data)
        with self.assertRaises(ValueError):
            verify(self.image, self.seed)


if __name__ == "__main__":
    unittest.main()
