"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Build the Harmony C extension and a source-pinned experimental Cellz DFU.

Offline only. Never imports a USB transport or flashes a device.
"""
import argparse
import shutil
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from firmware_container import Firmware

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
OUT = ROOT / 'build/harmony'
ZIG = None
SOURCE = None
SOURCE_SHA = 'f40a490705a33417e06fcf7b80f7eeb8bb78ebb6fbe894d5e3385b08d88e4ac9'
BASE, EXTENSION = 0x08003000, 0x08008800


def elf_contents(path):
    data = path.read_bytes()
    if data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('Require little-endian ELF32')
    header = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    if header[2] != 40:
        raise ValueError('Require ARM ELF')
    sections = [struct.unpack_from('<IIIIIIIIII', data, header[6] + i * header[11])
                for i in range(header[12])]
    payload = bytearray()
    symbols = {}
    for section in sections:
        _, typ, flags, address, offset, size, link, _, _, entry_size = section
        if flags & 2 and size:
            if typ != 1 or address < EXTENSION or address + size > EXTENSION + 0x4000:
                raise ValueError('Unexpected loadable section')
            end = address - EXTENSION + size
            if len(payload) < end:
                payload.extend(b'\xff' * (end - len(payload)))
            payload[address-EXTENSION:end] = data[offset:offset+size]
        if typ == 2:
            strings = sections[link]
            names = data[strings[4]:strings[4]+strings[5]]
            for pos in range(offset, offset + size, entry_size):
                name, value, _, _, _, _ = struct.unpack_from('<IIIBBH', data, pos)
                label = names[name:].split(b'\0', 1)[0].decode('ascii')
                if label:
                    symbols[label] = value
    return bytes(payload), symbols


def branch_link(address, target):
    target &= ~1
    delta = target - (address + 4)
    if delta % 2 or not -(1 << 24) <= delta < (1 << 24):
        raise ValueError('BL destination out of range')
    bits = delta & 0x1ffffff
    sign, i1, i2 = (bits >> 24) & 1, (bits >> 23) & 1, (bits >> 22) & 1
    j1, j2 = 1 ^ i1 ^ sign, 1 ^ i2 ^ sign
    return struct.pack('<HH', 0xf000 | sign << 10 | ((bits >> 12) & 0x3ff),
                       0xd000 | j1 << 13 | j2 << 11 | ((bits >> 1) & 0x7ff))


def repack_extended(source, image):
    if len(image) % 1024 or not len(source.image) <= len(image) <= 47 * 1024:
        raise ValueError('Application must be page aligned and within loader capacity')
    wrapped = image[:0x2400] + source.transport_block + image[0x2400:]
    headers = []
    for line in source.lines[:source.first_data_line]:
        body = line.rstrip(b'\r\n')
        if body.startswith(b'File1:'):
            body = f'File1:0/{sum(image) & 65535:04x}'.encode()
        elif body.startswith(b'Name:'):
            body = b'Name:Cellz_Harmony_P1_UNTESTED'
        headers.append(body)
    rows = [wrapped[i:i+64].hex().upper().encode() for i in range(0, len(wrapped), 64)]
    return b'\r\n'.join(headers + rows) + b'\r\n'


def main():
    global SOURCE, ZIG, OUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-firmware',type=Path,required=True,
                        help='Official Cellz V0.0.3.0 DFU downloaded separately')
    parser.add_argument('--zig',default=shutil.which('zig'),help='Zig 0.14.1 executable')
    parser.add_argument('--output',type=Path,default=OUT)
    args = parser.parse_args()
    if not args.zig:
        parser.error('Install Zig 0.14.1 separately or supply --zig')
    SOURCE, ZIG, OUT = args.base_firmware.resolve(), Path(args.zig).resolve(), args.output.resolve()
    if subprocess.check_output([str(ZIG),'version'],text=True).strip() != '0.14.1':
        raise ValueError('Reproducible prototype requires Zig 0.14.1')
    if hashlib.sha256(SOURCE.read_bytes()).hexdigest() != SOURCE_SHA:
        raise ValueError('Unexpected official source image')
    source = Firmware.read(SOURCE)
    OUT.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env['ZIG_GLOBAL_CACHE_DIR'] = str(ROOT / 'build/cache-global')
    env['ZIG_LOCAL_CACHE_DIR'] = str(ROOT / 'build/cache-local')
    elf = OUT / 'cellz_harmony.elf'
    command = [str(ZIG), 'cc', '-target', 'thumb-freestanding-eabi', '-mcpu=cortex_m3',
               '-mthumb', '-Oz', '-g', '-ffreestanding', '-fno-builtin',
               '-fno-stack-protector', '-ffunction-sections', '-fdata-sections',
               '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-nostdlib',
               '-Wl,--gc-sections', '-Wl,-e,harmony_boot', '-Wl,-T,' + str(ROOT / 'firmware/harmony.ld'),
               str(ROOT / 'firmware/cellz_harmony.c'), '-o', str(elf)]
    subprocess.run(command, env=env, cwd=ROOT, check=True)
    payload, symbols = elf_contents(elf)
    if not payload:
        raise ValueError('Empty extension')
    image = bytearray(source.image + payload)
    patches = []

    def patch(address, length, replacement, purpose):
        offset = address - BASE
        expected = source.image[offset:offset+length]
        if len(expected) != length or len(replacement) != length:
            raise ValueError('Patch length mismatch')
        image[offset:offset+len(replacement)] = replacement
        patches.append({'address': hex(address), 'before': expected.hex(),
                        'after': replacement.hex(), 'purpose': purpose})

    patch(0x080076e2, 4,
          branch_link(0x080076e2, symbols['harmony_boot']), 'Deterministic startup replaces stored-note restore')
    patch(0x08007820, 4,
          branch_link(0x08007820, symbols['harmony_tick']), 'Pots select shared harmony and paired layout')
    patch(0x08006a28, 2, b'\x70\x47', 'Tune buttons inactive; prevents old edits and mode changes')
    patch(0x08003ec6, 2, b'\x00\xbf', 'Disable old pot-to-arpeggio-tempo assignment')
    patch(0x08003ed6, 2, b'\x00\xbf', 'Disable old pot-to-extra-octaves assignment')
    image.extend(b'\xff' * (-len(image) % 1024))
    binary = OUT / 'Cellz_Harmony_P1.UNTESTED.bin'
    dfu = OUT / 'Cellz_Harmony_P1.UNTESTED.dfu'
    binary.write_bytes(image)
    dfu.write_bytes(repack_extended(source, bytes(image)))
    rebuilt = Firmware.read(dfu)
    if rebuilt.image != image or rebuilt.transport_block != source.transport_block or rebuilt.checksum != rebuilt.expected_checksum:
        raise ValueError('Repacked image did not validate')
    manifest = {'status': 'Locally generated experimental firmware; see docs/harmony.md',
                'source_sha256': SOURCE_SHA, 'image_sha256': hashlib.sha256(image).hexdigest(),
                'dfu_sha256': hashlib.sha256(dfu.read_bytes()).hexdigest(),
                'checksum': f'{rebuilt.checksum:04x}', 'bytes': len(image),
                'transmitted_blocks': len(rebuilt.wrapped) // 1024,
                'extension_address': hex(EXTENSION), 'extension_bytes': len(payload),
                'symbols': {k: hex(v) for k, v in symbols.items() if k.startswith('harmony_')},
                'patches': patches, 'bootloader_modified': False,
                'hardware_flashed': False, 'application_signature': 'HARMONY1', 'ram_state_range': ['0x2000026c', '0x20000270'],
                'c_source_sha256': hashlib.sha256((ROOT / 'firmware/cellz_harmony.c').read_bytes()).hexdigest(),
                'linker_source_sha256': hashlib.sha256((ROOT / 'firmware/harmony.ld').read_bytes()).hexdigest(),
                'compiler': 'Zig 0.14.1, official archive SHA256 554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c'}
    (OUT / 'build.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
