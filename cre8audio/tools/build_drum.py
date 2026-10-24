"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Offline build of separate Cellz Drum D1. Obtain official firmware and Zig separately.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
from build_harmony import elf_contents, branch_link, SOURCE_SHA, BASE
from firmware_container import Firmware

ROOT=Path(__file__).resolve().parent.parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-firmware',type=Path,required=True)
    parser.add_argument('--zig',default=shutil.which('zig'))
    parser.add_argument('--output',type=Path,default=ROOT/'build/drum')
    args=parser.parse_args()
    if not args.zig: parser.error('Supply Zig 0.14.1 with --zig or PATH')
    zig=Path(args.zig).resolve(); out=args.output.resolve()
    if subprocess.check_output([str(zig),'version'],text=True).strip()!='0.14.1':
        raise ValueError('Require Zig 0.14.1')
    if hashlib.sha256(args.base_firmware.read_bytes()).hexdigest()!=SOURCE_SHA:
        raise ValueError('Unexpected official source firmware')
    source=Firmware.read(args.base_firmware)
    out.mkdir(parents=True,exist_ok=True)
    env=os.environ.copy()
    env['ZIG_GLOBAL_CACHE_DIR']=str(ROOT/'build/cache-global')
    env['ZIG_LOCAL_CACHE_DIR']=str(ROOT/'build/cache-local')
    names=('cellz_drum.c','drum_engine.c','drum_store.c')
    elf=out/'cellz_drum.elf'
    command=[str(zig),'cc','-target','thumb-freestanding-eabi','-mcpu=cortex_m3',
             '-mthumb','-Oz','-g','-ffreestanding','-fno-builtin','-fno-stack-protector',
             '-ffunction-sections','-fdata-sections','-fno-unwind-tables',
             '-fno-asynchronous-unwind-tables','-nostdlib','-Wl,--gc-sections',
             '-Wl,-e,drum_boot','-Wl,-T,'+str(ROOT/'firmware/drum.ld')]
    subprocess.run(command+[str(ROOT/'firmware'/name) for name in names]+['-o',str(elf)],
                   cwd=ROOT,env=env,check=True)
    extension,symbols=elf_contents(elf)
    image=bytearray(source.image+extension)
    patches=[]
    def patch(address,length,replacement,purpose):
        offset=address-BASE
        if len(replacement)!=length or len(source.image[offset:offset+length])!=length:
            raise ValueError('Invalid patch extent')
        image[offset:offset+length]=replacement
        patches.append({'address':hex(address),'bytes':length,'purpose':purpose})
    for address,name in ((0x080076e2,'drum_boot'),(0x08007820,'drum_tick')):
        patch(address,4,branch_link(address,symbols[name]),name)
    for address in (0x080077fe,0x08007810,0x08007814,0x08007818,0x0800781c):
        patch(address,4,b'\x00\xbf'*2,'Disable old touch/editor/playback/gate side effects')
    for address in (0x08003ec6,0x08003ed6):
        patch(address,2,b'\x00\xbf','Keep ADC averaging; disable old pot assignments')
    for address,name in ((0x0800303c,'drum_systick'),(0x08003064,'drum_right_irq'),(0x080030e0,'drum_left_irq')):
        patch(address,4,struct.pack('<I',symbols[name]),name)
    image.extend(b'\xff'*(-len(image)%1024))
    # The loader erases one extra transport page. Keep both journal pages out
    # of that erase span, as well as out of the executable image.
    if BASE+len(image)+1024>0x0800e800:
        raise ValueError('Application/loader erase would overlap Drum storage')
    wrapped=image[:0x2400]+source.transport_block+image[0x2400:]
    headers=[]
    for line in source.lines[:source.first_data_line]:
        body=line.rstrip(b'\r\n')
        if body.startswith(b'File1:'): body=f'File1:0/{sum(image)&65535:04x}'.encode()
        elif body.startswith(b'Name:'): body=b'Name:Cellz_Drum_D1_UNTESTED'
        headers.append(body)
    rows=[wrapped[i:i+64].hex().upper().encode() for i in range(0,len(wrapped),64)]
    dfu=out/'Cellz_Drum_D1.UNTESTED.dfu'
    dfu.write_bytes(b'\r\n'.join(headers+rows)+b'\r\n')
    (out/'Cellz_Drum_D1.UNTESTED.bin').write_bytes(image)
    rebuilt=Firmware.read(dfu)
    if rebuilt.image!=image or rebuilt.checksum!=rebuilt.expected_checksum:
        raise ValueError('Repacked image does not validate')
    manifest={'status':'Experimental separate Drum D1; see docs/drum.md',
        'source_sha256':SOURCE_SHA,'image_sha256':hashlib.sha256(image).hexdigest(),
        'dfu_sha256':hashlib.sha256(dfu.read_bytes()).hexdigest(),
        'checksum':f'{rebuilt.checksum:04x}','bytes':len(image),'transmitted_blocks':len(wrapped)//1024,
        'application_signature':'DRUM0001','bootloader_modified':False,'hardware_flashed':False,
        'extension_bytes':len(extension),'symbols':{k:hex(v) for k,v in symbols.items() if k.startswith('drum_')},
        'patches':patches,'pattern_pages':['0x0800e800','0x0800ec00'],
        'factory_settings_region_untouched':['0x0800f000','0x08010000'],
        'ram_state_range':['0x20001200','0x20001600'],
        'source_files':{name:hashlib.sha256((ROOT/'firmware'/name).read_bytes()).hexdigest()
                        for name in (*names,'drum_engine.h','drum_store.h','drum.ld')}}
    (out/'build.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))

if __name__=='__main__': main()
