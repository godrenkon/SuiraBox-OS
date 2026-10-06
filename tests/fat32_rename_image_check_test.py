import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from fat32_rename_image_check import PAYLOAD, verify
from fat32_mkdir_image_check import raw_entry
from fat32_recovery_image_check import CLEAN, NO_ERROR


class RenameImageCheckTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.seed, self.image = Path(tmp.name) / "seed.img", Path(tmp.name) / "image.img"
        data = bytearray(64 * 512)
        struct.pack_into("<H", data, 11, 512)
        data[13], data[16] = 1, 2
        struct.pack_into("<H", data, 14, 2)
        for offset, value in ((32, 64), (36, 1), (44, 2)):
            struct.pack_into("<I", data, offset, value)
        struct.pack_into("<H", data, 48, 1)
        data[510:512] = b"\x55\xaa"
        for offset, value in ((0, 0x41615252), (484, 0x61417272), (488, 50), (492, 4), (508, 0xAA550000)):
            struct.pack_into("<I", data, 512 + offset, value)
        for copy in range(2):
            for cluster in range(4):
                struct.pack_into("<I", data, (copy + 2) * 512 + cluster * 4,
                                 (0xA0000000 if copy == 0 else 0xB0000000) | 0x0FFFFFFF)
        data[4 * 512:4 * 512 + 32] = raw_entry(b"RUNTIME TXT", 0x20, 3, 5)
        data[5 * 512:5 * 512 + 5] = b"hello"
        self.seed.write_bytes(data)
        data[4 * 512 + 32:4 * 512 + 64] = raw_entry(b"WORLD   DAT", 0x20, 4, len(PAYLOAD) + 1)
        data[4 * 512 + 64:4 * 512 + 96] = raw_entry(b"SAVES      ", 0x10, 5)
        data[7 * 512:7 * 512 + 96] = (raw_entry(b".          ", 0x10, 5) +
            raw_entry(b"..         ", 0x10, 0) + raw_entry(b"WORLD   DAT", 0x20, 6, len(PAYLOAD)))
        data[6 * 512:6 * 512 + len(PAYLOAD) + 1] = PAYLOAD + b"!"
        data[8 * 512:8 * 512 + len(PAYLOAD)] = PAYLOAD
        for copy in range(2):
            for cluster in (4, 5, 6):
                struct.pack_into("<I", data, (copy + 2) * 512 + cluster * 4, 0x0FFFFFFF)
        struct.pack_into("<II", data, 512 + 488, 0xFFFFFFFF, 0xFFFFFFFF)
        self.good = bytes(data)
        self.image.write_bytes(data)

    def test_exact_renamed_files_pass(self):
        with contextlib.redirect_stdout(io.StringIO()):
            verify(self.image, self.seed)

    def test_old_name_wrong_metadata_or_slack_rejected(self):
        for offset in (4 * 512 + 32, 4 * 512 + 60, 7 * 512 + 64, 7 * 512 + 100, 6 * 512 + 25):
            with self.subTest(offset=offset):
                data = bytearray(self.good)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.seed)

    def test_fat_status_reserved_bits_and_other_data_rejected(self):
        for offset in (2 * 512 + 4, 3 * 512 + 7, 3 * 512 + 16, 5 * 512, 8 * 512, len(self.good) - 1):
            with self.subTest(offset=offset):
                data = bytearray(self.good)
                data[offset] ^= 1
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.seed)
        for mask in (CLEAN, NO_ERROR):
            for copy in range(2):
                data = bytearray(self.good)
                offset = (copy + 2) * 512 + 4
                struct.pack_into("<I", data, offset, struct.unpack_from("<I", data, offset)[0] ^ mask)
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.seed)

    def test_alias_to_seed_or_other_new_file_rejected(self):
        for cluster in (3, 4, 5):
            with self.subTest(cluster=cluster):
                data = bytearray(self.good)
                struct.pack_into("<H", data, 7 * 512 + 64 + 26, cluster)
                self.image.write_bytes(data)
                with self.assertRaises(ValueError):
                    verify(self.image, self.seed)

    def test_bad_dot_entry_or_fsinfo_rejected(self):
        for offset in (7 * 512 + 26, 7 * 512 + 32 + 26, 512 + 488, 512 + 492):
            data = bytearray(self.good)
            data[offset] ^= 1
            self.image.write_bytes(data)
            with self.assertRaises(ValueError):
                verify(self.image, self.seed)

    def test_bad_cluster_or_truncated_image_rejected(self):
        data = bytearray(self.good)
        struct.pack_into("<H", data, 4 * 512 + 64 + 26, 200)
        self.image.write_bytes(data)
        with self.assertRaises(ValueError):
            verify(self.image, self.seed)
        self.image.write_bytes(self.good[:-1])
        with self.assertRaises(ValueError):
            verify(self.image, self.seed)


if __name__ == "__main__":
    unittest.main()
