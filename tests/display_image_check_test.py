import contextlib
import io
import unittest
from display_image_check import expected_pixels, read_ppm, verify_bytes


class DisplayImageCheckTests(unittest.TestCase):
    def test_complete_proof_image_passes(self):
        for width, height in ((640, 480), (1024, 768)):
            with contextlib.redirect_stdout(io.StringIO()):
                verify_bytes(f"P6\n{width} {height}\n255\n".encode() + expected_pixels(width, height))

    def test_wrong_color_boundaries_or_failed_request_write_rejected(self):
        original = expected_pixels(640, 480)
        for x, y in ((0, 0), (159, 72), (160, 72), (184, 72), (335, 136), (336, 136), (512, 263), (615, 457), (639, 479)):
            with self.subTest(x=x, y=y):
                pixels = bytearray(original)
                pixels[(y * 640 + x) * 3] ^= 1
                with self.assertRaises(ValueError):
                    verify_bytes(b"P6\n640 480\n255\n" + pixels)

    def test_malformed_header_or_payload_rejected(self):
        for data in (b"P3\n1 1\n255\nabc", b"P6\n1 1\n256\nabc", b"P6\n0 1\n255\n", b"P6\n4097 1\n255\n",
                     b"P6\n1 1\n255\n", b"P6\n1 1\n255\nabcX", b"P6\n# unterminated", b"P6\n1 1\n255"):
            with self.subTest(data=data), self.assertRaises(ValueError):
                read_ppm(data)

    def test_comments_and_whitespace_valued_first_pixel(self):
        for separator in (b"\n", b"\r\n"):
            self.assertEqual(read_ppm(b"P6\n# test\n1 1\n255" + separator + b"\n #"), (1, 1, b"\n #"))

    def test_small_mode_rejected(self):
        with self.assertRaises(ValueError):
            verify_bytes(b"P6\n1 1\n255\nabc")


if __name__ == "__main__":
    unittest.main()
