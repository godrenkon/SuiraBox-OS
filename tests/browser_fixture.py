"""Write disposable host fixture files before mtools creates the CI volume."""
from pathlib import Path
import sys
p=Path(sys.argv[1])
files={'LEVEL.TXT':b'SuiraBox world metadata\nSeed: 12345\n',
       'README.TXT':b'SuiraBox worlds\nRead-only preview.\r\nTabs:\tOK\n',
       'EMPTY.TXT':b'', 'LONG.TXT':b'A'*600,
       'BINARY.DAT':b'\0ELF\x01\n\xffZ\tEND\r\n'}
for name,data in files.items(): (p/name).write_bytes(data)
