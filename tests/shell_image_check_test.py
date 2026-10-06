import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from shell_image_check import expected, verify

class ShellScreenTests(unittest.TestCase):
    def test_view_clears_previous_content(self):
        home=expected(640,480,'Home'); files=expected(640,480,'Files',[('RUNTIME.TXT',16)],True)
        self.assertNotEqual(home,files)
        # Home welcome ink is cleared when visiting the Files view.
        offset=(128*640+208)*3
        self.assertEqual(files[offset:offset+3],bytes((12,16,24)))
    def test_corruption_and_wrong_view_detected(self):
        with TemporaryDirectory() as directory:
            path=Path(directory)/'screen.ppm'
            pixels=bytearray(expected(640,480,'Home'))
            path.write_bytes(b'P6\n640 480\n255\n'+pixels)
            verify(path,'Home')
            with self.assertRaises(ValueError): verify(path,'Settings')
            pixels[-1]^=1
            path.write_bytes(b'P6\n640 480\n255\n'+pixels)
            with self.assertRaises(ValueError): verify(path,'Home')
    def test_file_size_is_visible(self):
        self.assertNotEqual(expected(640,480,'Files',[('user-hello',123)]),
                            expected(640,480,'Files',[('user-hello',124)]))

if __name__ == '__main__': unittest.main()
