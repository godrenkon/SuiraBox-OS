#!/usr/bin/env python3
"""Independent full-frame expectations for the real desktop and VFS fixture."""
from pathlib import Path
import sys
from display_image_check import read_ppm, paint_text

BG, PANEL, FG, ACCENT = 0x0c1018, 0x152536, 0xe6eef2, 0x259b72

def expected(width, height, view, entries=(), disk=False):
    if width < 640 or height < 480 or view not in ('Home', 'Files', 'Settings'):
        raise ValueError('unsupported shell expectation')
    pixels = bytearray(BG.to_bytes(3, 'big') * width * height)
    def fill(x, y, w, h, color):
        row = color.to_bytes(3, 'big') * w
        for yy in range(y, y+h):
            offset = (yy*width+x)*3
            pixels[offset:offset+len(row)] = row
    fill(0,0,width,48,PANEL)
    fill(16,72,160,height-112,PANEL)
    fill(0,height-28,width,28,PANEL)
    labels = [(24,16,'SuiraBox OS',FG,PANEL,2), (208,80,view,FG,BG,2)]
    for index, name in enumerate(('Home','Files','Settings')):
        color = ACCENT if name == view else PANEL
        fill(24,88+index*40,144,32,color)
        labels.append((32,96+index*40,name,FG,color,2))
    footer = 'F1 Home  F2 Files  F3 Settings  Tab cycle  Esc Home'
    if view == 'Home':
        labels += [(208,128,'Welcome to SuiraBox',FG,BG,2),
                   (208,176,'Kernel, storage and keyboard are ready.',FG,BG,1),
                   (208,200,'Use F2 to browse boot files or the disk.',FG,BG,1),
                   (208,248,'Minecraft runtime: not installed',0xf2ae43,BG,1),
                   (208,272,'Network, JVM and game launcher are pending.',FG,BG,1)]
    elif view == 'Settings':
        labels += [(208,128,f'Display: {width}x{height}',FG,BG,1),
                   (208,160,'Keyboard: PS/2, US layout',FG,BG,1),
                   (208,192,'Text: ASCII bitmap, UTF-8 fallback',FG,BG,1),
                   (208,224,'CJK fonts and IME are pending.',0xf2ae43,BG,1),
                   (208,256,'Persistent settings are pending.',FG,BG,1)]
    else:
        footer = 'B boot  D disk  R refresh  F1 Home  F3 Settings'
        labels += [(208,112,'/disk' if disk else '/boot',ACCENT,BG,2),
                   (208,140,'Type / name                  Size (bytes)',FG,BG,1)]
        for index, (name, size) in enumerate(entries):
            labels.append((208,160+index*24,f'[F] {name:<24} {size}',FG,BG,1))
    labels.append((24,height-20,footer,FG,PANEL,1))
    paint_text(pixels,width,height,labels)
    return bytes(pixels)

def verify(path, view, entries=(), disk=False):
    width,height,actual = read_ppm(path.read_bytes())
    wanted = expected(width,height,view,entries,disk)
    if actual != wanted:
        pixel = next(i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b)//3
        raise ValueError(f'{path.name}: shell mismatch at ({pixel%width},{pixel//width})')
    print(f'QEMU shell screen OK: {path.name}, exact {width}x{height} RGB, {view}')

def verify_lifecycle(build):
    log = (build/'display.log').read_text()
    if 'FAILED' in log or 'Exception:' in log:
        raise ValueError('guest shell lifecycle failed')
    frames = [(1,'Home'),(2,'Files /boot'),(3,'Files /disk'),(4,'Files /disk'),
              (5,'Settings'),(6,'Home'),(7,'Settings'),(8,'Home'),(9,'Files /disk'),(10,'Files /boot')]
    cursor = 0
    for number,view in frames:
        marker = f'Userspace: shell frame {number} {view}\n'
        position = log.find(marker,cursor)
        if position < 0: raise ValueError(f'missing completed frame: {marker}')
        cursor = position+len(marker)
    boot = [('user-hello',(build/'user-hello.elf').stat().st_size),
            ('user-child',(build/'user-child.elf').stat().st_size)]
    disk = [('RUNTIME.TXT',(build.parent/'runtime.txt').stat().st_size)]
    for name in ('home','home-cycle','home-escape'): verify(build/f'{name}.ppm','Home')
    for name in ('boot-files','boot-return'): verify(build/f'{name}.ppm','Files',boot)
    for name in ('disk-files','disk-refresh','disk-return'): verify(build/f'{name}.ppm','Files',disk,True)
    for name in ('settings','settings-reverse'): verify(build/f'{name}.ppm','Settings')
    print('QEMU desktop navigation and read-only VFS snapshots lifecycle OK: 10 completed frames')

if __name__ == '__main__':
    try:
        if '--home' in sys.argv[2:]: verify(Path(sys.argv[1]),'Home')
        else: verify_lifecycle(Path(sys.argv[1]))
    except (OSError,ValueError,IndexError) as error:
        raise SystemExit(str(error)) from error
