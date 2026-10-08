#!/usr/bin/env python3
"""Distinct 8.3 files used by the read-only multi-page desktop proof."""
from pathlib import Path
import sys

def create(directory):
    directory.mkdir(parents=True, exist_ok=True)
    for number in range(1, 28):
        (directory/f'FILE{number:02d}.TXT').write_bytes(f'SuiraBox file {number:02d}\n'.encode('ascii'))

if __name__ == '__main__':
    create(Path(sys.argv[1]))
