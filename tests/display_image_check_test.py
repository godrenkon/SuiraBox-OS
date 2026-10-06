import contextlib
import io
import unittest
from display_image_check import expected_pixels, read_ppm, verify_bytes, font_rows, paint_text


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

    def test_actual_text_proof_and_normal_boot_modes(self):
        for width, height in ((640, 480), (1024, 768)):
            for text, boot in ((True, False), (False, True)):
                with contextlib.redirect_stdout(io.StringIO()):
                    verify_bytes(f"P6\n{width} {height}\n255\n".encode() + expected_pixels(width, height, text, boot), text, boot)

    def test_glyph_damage_clipping_or_partial_invalid_draw_detected(self):
        original = expected_pixels(640, 480, True)
        for x, y in ((30,16), (201,82), (208,312), (212,344), (0,54), (635,52), (2,475), (0,0)):
            with self.subTest(x=x, y=y):
                pixels = bytearray(original)
                pixels[(y * 640 + x) * 3] ^= 1
                with self.assertRaises(ValueError):
                    verify_bytes(b"P6\n640 480\n255\n" + pixels, True)

    def test_known_font_artwork_and_clipped_replacement(self):
        rows = font_rows()
        self.assertEqual(rows[ord("A")-32], (14,17,17,31,17,17,17))
        self.assertNotEqual(rows[ord("a")-32], rows[ord("A")-32])
        self.assertTrue(all(any(row) for row in rows[1:]))
        pixels = bytearray(8 * 8 * 3)
        paint_text(pixels, 8, 8, [(-3,-5,"日",0xffffff,0x000001,2)])
        full = bytearray(16 * 16 * 3)
        paint_text(full, 16, 16, [(0,0,"?",0xffffff,0x000001,2)])
        for y in range(8):
            for x in range(8):
                self.assertEqual(pixels[(y*8+x)*3:(y*8+x+1)*3],
                                 full[((y+5)*16+x+3)*3:((y+5)*16+x+4)*3])

    def test_keyboard_input_before_and_after(self):
        for width, height in ((640,480), (1024,768)):
            before = expected_pixels(width,height,boot_text=True)
            after = expected_pixels(width,height,input_proof=True)
            self.assertNotEqual(before,after)
            with contextlib.redirect_stdout(io.StringIO()):
                verify_bytes(f"P6\n{width} {height}\n255\n".encode()+after,input_proof=True)
            with self.assertRaises(ValueError):
                verify_bytes(f"P6\n{width} {height}\n255\n".encode()+before,input_proof=True)

    def test_wrong_submitted_text_or_undelivered_enter_detected(self):
        original = expected_pixels(640,480,input_proof=True)
        for x,y in ((286,144),(288,146),(24,112),(30,80)):
            pixels=bytearray(original)
            pixels[(y*640+x)*3]^=1
            with self.assertRaises(ValueError):
                verify_bytes(b"P6\n640 480\n255\n"+pixels,input_proof=True)


if __name__ == "__main__":
    unittest.main()
