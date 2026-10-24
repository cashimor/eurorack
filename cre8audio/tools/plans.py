"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Validate locally supplied firmware; no vendor firmware is distributed here.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
from firmware_container import Firmware

OFFICIAL_CELLZ = 'f40a490705a33417e06fcf7b80f7eeb8bb78ebb6fbe894d5e3385b08d88e4ac9'


@dataclass(frozen=True)
class Plan:
    source_sha256: str
    blocks: tuple
    image: bytes
    checksum: int

    @classmethod
    def from_file(cls, path, manifest=None):
        fw = Firmware.read(path)
        metadata = dict(line.strip().decode('ascii').split(':', 1)
                        for line in fw.lines[:fw.first_data_line] if b':' in line)
        if any(metadata.get(key, '').lower() != value for key, value in
               [('VID','1acc'), ('APP_PID','1a4d'), ('DFU_PID','1a59')]):
            raise ValueError('File is not a supported Cellz application container')
        digest = hashlib.sha256(fw.original).hexdigest()
        if digest != OFFICIAL_CELLZ:
            if manifest is None:
                raise ValueError('Custom firmware requires its locally generated --manifest')
            build = json.loads(Path(manifest).read_text())
            if (build.get('source_sha256') != OFFICIAL_CELLZ
                    or build.get('dfu_sha256') != digest
                    or build.get('image_sha256') != hashlib.sha256(fw.image).hexdigest()
                    or build.get('checksum') != f'{fw.checksum:04x}'):
                raise ValueError('Custom image does not match its build manifest')
        if fw.checksum != fw.expected_checksum:
            raise ValueError('Container checksum does not match image')
        plan = cls(digest, tuple(fw.wrapped[i:i+1024] for i in range(0,len(fw.wrapped),1024)),
                   fw.image, fw.checksum)
        plan.validate()
        return plan

    def validate(self):
        if not 11 <= len(self.blocks) <= 48 or any(len(b) != 1024 for b in self.blocks):
            raise ValueError('Unsupported block count/size')
        normalized = b''.join(self.blocks[:9] + self.blocks[10:])
        if normalized != self.image or sum(normalized) & 65535 != self.checksum:
            raise ValueError('Plan content/checksum changed')
        stack, reset = (int.from_bytes(self.image[i:i+4], 'little') for i in (0,4))
        if not 0x20000000 < stack <= 0x20005000 or not reset & 1:
            raise ValueError('Unexpected application vectors')
        if not 0x08003000 <= reset & ~1 < 0x08003000 + len(self.image):
            raise ValueError('Reset vector lies outside application')
