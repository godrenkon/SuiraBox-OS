import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from shell_image_check import expected, verify, preview_rows

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
    def test_binary_and_whitespace_preview(self):
        rows, clipped=preview_rows(b'A\r\nB\tC\x00\xffZ')
        self.assertEqual([r.rstrip() for r in rows],['A','B   C..Z'])
        self.assertFalse(clipped)
        rows,clipped=preview_rows(b'\t'*512)
        self.assertEqual(len(rows),12)
        self.assertTrue(clipped)
        self.assertEqual(preview_rows(b''),([],False))
    def test_selected_row_and_preview_are_distinct(self):
        entries=[('WORLDS',0,'D'),('EMPTY.TXT',0)]
        self.assertNotEqual(expected(640,480,'Files',entries,selected=0),
                            expected(640,480,'Files',entries,selected=1))
        self.assertNotEqual(expected(640,480,'Files',entries),
                            expected(640,480,'Files',entries,preview=b'',filename='EMPTY.TXT'))
    def test_page_range_is_checked_independently(self):
        entries=[(f'FILE{i:02d}.TXT',17) for i in range(13,25)]
        second=expected(640,480,'Files',entries,offset=12,more=True)
        self.assertNotEqual(second,expected(640,480,'Files',entries,more=True))
        with TemporaryDirectory() as directory:
            path=Path(directory)/'page.ppm'
            path.write_bytes(b'P6\n640 480\n255\n'+second)
            verify(path,'Files',entries,offset=12,more=True)
            with self.assertRaises(ValueError): verify(path,'Files',entries,more=True)
    def test_short_last_page_clears_previous_rows(self):
        entries=[(f'FILE{i:02d}.TXT',17) for i in range(25,28)]
        third=expected(640,480,'Files',entries,offset=24)
        for y in range(232,440):
            self.assertEqual(third[(y*640+208)*3:(y*640+608)*3],bytes((12,16,24))*400)

if __name__ == '__main__': unittest.main()
