#!/usr/bin/env python3
"""Independent full-frame expectations for the real desktop and VFS fixture."""
from pathlib import Path
import sys
from display_image_check import read_ppm, paint_text

BG, PANEL, FG, ACCENT = 0x0c1018, 0x152536, 0xe6eef2, 0x259b72

def preview_rows(data):
    # Byte-oriented, printable ASCII preview: binary/CJK bytes become periods.
    rows=['']; index=0
    while index<len(data) and len(rows)<=12:
        byte=data[index]; index+=1
        if byte in (10,13):
            if byte==13 and index<len(data) and data[index]==10: index+=1
            rows.append(''); continue
        if len(rows[-1])==48: rows.append('')
        if len(rows)>12: index-=1; break
        if byte==9: rows[-1]+=' '*(4-len(rows[-1])%4)
        else: rows[-1]+=chr(byte) if 32<=byte<=126 else '.'
    return [r.ljust(48) for r in rows[:12]] if data else [], index<len(data)

def expected(width, height, view, entries=(), disk=False, selected=0, location=None, preview=None, filename=None, offset=0, more=False):
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
    title=view
    if view=='Files' and preview is None and entries and (offset or more):
        title=f'Files {offset+1}-{offset+len(entries)}'+('+' if more else '')
    labels = [(24,16,'SuiraBox OS',FG,PANEL,2), (208,80,title,FG,BG,2 if len(title)<=24 else 1)]
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
        footer = 'Up/Down select  Enter open  Backspace up  B/D root  R reload'
        if offset or more: footer='PgUp/PgDn pages  Up/Down select  Enter open  Backspace up'
        path=location or ('/disk' if disk else '/boot')
        labels.append((208,112,path,ACCENT,BG,2))
        if preview is not None:
            labels.append((208,140,f'Preview: {filename}',FG,BG,1))
            rows, clipped=preview_rows(preview[:512])
            if not rows: labels.append((208,176,'File is empty',FG,BG,1))
            for index,row in enumerate(rows): labels.append((208,160+index*24,row,FG,BG,1))
            footer = ('Preview truncated' if clipped or len(preview)>512 else 'Read-only preview')+'  Esc/Backspace list  B/D root'
        else:
            labels.append((208,140,'Type / name                  Size (bytes)',FG,BG,1))
            for index, entry in enumerate(entries):
                name,size=entry[:2]; kind=entry[2] if len(entry)>2 else 'F'
                color=0x18394b if index==selected else BG
                if index==selected: fill(208,160+index*24,400,16,color)
                labels.append((208,160+index*24,f'[{kind}] {name:<24} {size}',FG,color,1))
    labels.append((24,height-20,footer,FG,PANEL,1))
    paint_text(pixels,width,height,labels)
    return bytes(pixels)

def verify(path, view, entries=(), disk=False, **options):
    width,height,actual = read_ppm(path.read_bytes())
    wanted = expected(width,height,view,entries,disk,**options)
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

def verify_browser(build):
    log=(build/'display.log').read_text()
    if 'FAILED' in log or 'Exception:' in log: raise ValueError('guest browser failure')
    tail=log.find('Userspace: runtime FAT32 additional directory entries verified')
    complete=log.find('Userspace: runtime FAT32 directory enumeration OK',tail)
    ready=log.find('Userspace: shell ready',complete)
    if tail<0 or complete<0 or ready<0:
        raise ValueError('browser boot did not enumerate additional root entries through EOF')
    boot=[('user-hello',(build/'user-hello.elf').stat().st_size),('user-child',(build/'user-child.elf').stat().st_size)]
    root=[('RUNTIME.TXT',(build.parent/'runtime.txt').stat().st_size),('SAVES',0,'D')]
    saves=[('WORLDS',0,'D')]+[(n,(build/n).stat().st_size) for n in ('README.TXT','EMPTY.TXT','LONG.TXT','BINARY.DAT')]
    worlds=[('LEVEL.TXT',(build/'LEVEL.TXT').stat().st_size)]
    stages=[('home','Home',[],{}),('boot-files','Files',boot,{'location':'/boot'}),
            ('disk-files','Files',root,{'location':'/disk'}),('select-saves','Files',root,{'location':'/disk','selected':1}),
            ('saves','Files',saves,{'location':'/disk/SAVES'}),('worlds','Files',worlds,{'location':'/disk/SAVES/WORLDS'}),
            ('level-preview','Files',worlds,{'location':'/disk/SAVES/WORLDS','filename':'LEVEL.TXT','preview':(build/'LEVEL.TXT').read_bytes()}),
            ('worlds-return','Files',worlds,{'location':'/disk/SAVES/WORLDS'}),
            ('saves-return','Files',saves,{'location':'/disk/SAVES'})]
    for index,(name,stem) in enumerate([('README.TXT','readme'),('EMPTY.TXT','empty'),('LONG.TXT','long'),('BINARY.DAT','binary')],1):
        options={'location':'/disk/SAVES','selected':index}
        stages += [(f'select-{stem}','Files',saves,options),
                   (f'{stem}-preview','Files',saves,{**options,'filename':name,'preview':(build/name).read_bytes()}),
                   (f'{stem}-return','Files',saves,options)]
    stages += [('root-return','Files',root,{'location':'/disk'}),('boot-return','Files',boot,{'location':'/boot'}),
               ('elf-preview','Files',boot,{'location':'/boot','filename':'user-hello','preview':(build/'user-hello.elf').read_bytes()}),
               ('elf-return','Files',boot,{'location':'/boot'}),('home-return','Home',[],{})]
    cursor=0
    for number,(name,view,entries,options) in enumerate(stages,1):
        marker=f'Userspace: shell frame {number} {view}'
        if view=='Files': marker+=' '+options['location']+(' preview' if 'preview' in options else '')
        marker+='\n'; position=log.find(marker,cursor)
        if position<0: raise ValueError(f'missing completed browser frame: {marker}')
        cursor=position+len(marker)
        verify(build/f'{name}.ppm',view,entries,**options)
    print(f'QEMU Files browser lifecycle OK: {len(stages)} frames, nested paths and real text/empty/long/binary/ELF reads')

def verify_paging(build):
    log=(build/'display.log').read_text()
    if 'FAILED' in log or 'Exception:' in log: raise ValueError('guest paging failure')
    boot=[('user-hello',(build/'user-hello.elf').stat().st_size),('user-child',(build/'user-child.elf').stat().st_size)]
    root=[('RUNTIME.TXT',(build.parent/'runtime.txt').stat().st_size),('MANY',0,'D')]
    files=[(f'FILE{i:02d}.TXT',(build/f'FILE{i:02d}.TXT').stat().st_size) for i in range(1,28)]
    first,second,third=files[:12],files[12:24],files[24:]
    a={'location':'/disk/MANY','more':True}
    b={**a,'offset':12}
    c={'location':'/disk/MANY','offset':24}
    def preview(options,number):
        name=f'FILE{number:02d}.TXT'
        return {**options,'filename':name,'preview':(build/name).read_bytes()}
    stages=[('home','Home',[],{}),('boot-files','Files',boot,{'location':'/boot'}),
            ('disk-files','Files',root,{'location':'/disk'}),('select-many','Files',root,{'location':'/disk','selected':1}),
            ('page-one','Files',first,a),('page-two','Files',second,b),
            ('file13-preview','Files',second,preview(b,13)),('file13-return','Files',second,b),
            ('page-three','Files',third,c),('file25-preview','Files',third,preview(c,25)),
            ('file25-return','Files',third,c),('select26','Files',third,{**c,'selected':1}),
            ('select27','Files',third,{**c,'selected':2}),('select26-return','Files',third,{**c,'selected':1}),
            ('page-two-return','Files',second,b),('page-one-last','Files',first,{**a,'selected':11}),
            ('file12-preview','Files',first,preview({**a,'selected':11},12)),
            ('file12-return','Files',first,{**a,'selected':11}),('page-two-edge','Files',second,b),
            ('page-two-refresh','Files',second,b),('root-return','Files',root,{'location':'/disk'}),
            ('boot-return','Files',boot,{'location':'/boot'}),('home-return','Home',[],{})]
    cursor=0
    for number,(name,view,entries,options) in enumerate(stages,1):
        marker=f'Userspace: shell frame {number} {view}'
        if view=='Files': marker+=' '+options['location']+(' preview' if 'preview' in options else '')
        position=log.find(marker+'\n',cursor)
        if position<0: raise ValueError(f'missing completed paging frame: {marker}')
        cursor=position+len(marker)+1
        verify(build/f'{name}.ppm',view,entries,**options)
    print(f'QEMU Files paging lifecycle OK: {len(stages)} frames, 27 entries, Page/arrow boundaries and files 12/13/25 previewed')

if __name__ == '__main__':
    try:
        if '--home' in sys.argv[2:]: verify(Path(sys.argv[1]),'Home')
        elif '--browser' in sys.argv[2:]: verify_browser(Path(sys.argv[1]))
        elif '--paging' in sys.argv[2:]: verify_paging(Path(sys.argv[1]))
        else: verify_lifecycle(Path(sys.argv[1]))
    except (OSError,ValueError,IndexError) as error:
        raise SystemExit(str(error)) from error
