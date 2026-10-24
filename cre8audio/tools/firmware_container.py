# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk.
"""Known single-image Tool Version V0.0.0.2 containers; no USB operations."""
from dataclasses import dataclass
from pathlib import Path
import re

BLOCK_OFFSET = 0x2400
BLOCK_SIZE = 0x400

@dataclass
class Firmware:
    original: bytes
    lines: list
    first_data_line: int
    wrapped: bytes
    expected_checksum: int

    @classmethod
    def read(cls, path):
        original = Path(path).read_bytes()
        lines = original.splitlines(keepends=True)
        first = next(i for i, line in enumerate(lines)
                     if re.fullmatch(rb'[0-9a-fA-F]{8,}', line.strip()))
        headers = b''.join(lines[:first]).decode('ascii').splitlines()
        if 'Tool Version:V0.0.0.2' not in headers or 'Count Sum:1' not in headers:
            raise ValueError('Only the known single-image Tool Version V0.0.0.2 format is supported')
        check = next(s for s in headers if s.startswith('File1:'))
        if not check.startswith('File1:0/'):
            raise ValueError('Unsupported target index')
        rows = [line.strip() for line in lines[first:]]
        if any(not re.fullmatch(rb'(?:[0-9a-fA-F]{2})+', row) for row in rows):
            raise ValueError('Unexpected hex body')
        wrapped = bytes.fromhex(b''.join(rows).decode('ascii'))
        if len(wrapped) <= BLOCK_OFFSET + BLOCK_SIZE or len(wrapped) % BLOCK_SIZE:
            raise ValueError('Unexpected payload length')
        return cls(original, lines, first, wrapped, int(check.split('/')[1], 16))

    @property
    def image(self):
        return self.wrapped[:BLOCK_OFFSET] + self.wrapped[BLOCK_OFFSET+BLOCK_SIZE:]

    @property
    def transport_block(self):
        return self.wrapped[BLOCK_OFFSET:BLOCK_OFFSET+BLOCK_SIZE]

    @property
    def checksum(self):
        return sum(self.image) & 0xffff

    def repack(self, image):
        if self.checksum != self.expected_checksum:
            raise ValueError('Source checksum does not validate')
        if len(image) != len(self.image):
            raise ValueError('Only same-size edits are supported')
        wrapped = image[:BLOCK_OFFSET] + self.transport_block + image[BLOCK_OFFSET:]
        if image == self.image:
            return self.original
        checksum = sum(image) & 0xffff
        result = []
        position = 0
        for index, line in enumerate(self.lines):
            body = line.rstrip(b'\r\n')
            ending = line[len(body):]
            if index < self.first_data_line:
                if body.startswith(b'File1:'):
                    result.append(f'File1:0/{checksum:04x}'.encode() + ending)
                else:
                    result.append(line)
            else:
                count = len(body) // 2
                result.append(wrapped[position:position+count].hex().upper().encode() + ending)
                position += count
        assert position == len(wrapped)
        return b''.join(result)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+', type=Path)
    args = parser.parse_args()
    for path in args.files:
        firmware = Firmware.read(path)
        print(f'{path.name}: expected={firmware.expected_checksum:04x}, computed={firmware.checksum:04x}, '
              f'image={len(firmware.image)} bytes, valid={firmware.checksum == firmware.expected_checksum}')
        if firmware.checksum != firmware.expected_checksum:
            raise SystemExit(1)
