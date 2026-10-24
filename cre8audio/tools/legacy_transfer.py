# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use. Use at your own risk.
"""Legacy migration transfer engine, transport-injected; no USB backend.

Default CLI only exports an offline plan. Does not enter update mode, launch
the migration application, reset devices, or install drivers.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import time
from legacy_container import legacy_parse

EXPECTED = '49a821eddd00c41f41980606ebeab708bef21b251ba142b0c9743034c0820492'


class TransferError(RuntimeError):
    pass


@dataclass(frozen=True)
class Plan:
    source_sha256: str
    blocks: tuple

    @classmethod
    def official_migration(cls, path):
        digest = hashlib.sha256(Path(path).read_bytes()).hexdigest()
        if digest != EXPECTED:
            raise ValueError('Only the exact supplied official migration installer is supported')
        _, vid, dfu_pid, app_pid, images = legacy_parse(path)
        if (vid, dfu_pid, app_pid) != (0x1acc, 0x1a4d, 0x1a4d) or len(images) != 1:
            raise ValueError('Unexpected migration metadata')
        raw = images[0]
        blocks = tuple(raw[i:i+1024].ljust(1024, b'\xff') for i in range(0, len(raw), 1024))
        if len(blocks) != 16:
            raise ValueError('Unexpected migration block count')
        return cls(digest, blocks)

    def validated(self):
        return (self.source_sha256 == EXPECTED and len(self.blocks) == 16
                and all(len(b) == 1024 for b in self.blocks)
                and hashlib.sha256(b''.join(self.blocks)).hexdigest() ==
                '44616d224da8ef55648529d50132c73c025b15a75e12507d9c1c6632f9ccbd71')

    @staticmethod
    def header(command, index=0, checksum=0):
        return bytes([command]) + index.to_bytes(2, 'little') + checksum.to_bytes(2, 'little') + b'\xff' * 11

    def block_header(self, index):
        return self.header(0x32, index, sum(self.blocks[index]) & 0xffff)

    def end_header(self):
        # Mac setupEndingState retains/checks the last payload's checksum.
        return self.header(0x34, len(self.blocks), sum(self.blocks[-1]) & 0xffff)

    def describe(self):
        return {'scope': 'Offline host plan; not transmitted or device-validated',
                'source_sha256': self.source_sha256,
                'initialize': self.header(0x33).hex(' '),
                'blocks': [{'index': i, 'payload_sha256': hashlib.sha256(b).hexdigest(),
                            'header': self.block_header(i).hex(' ')} for i, b in enumerate(self.blocks)],
                'end': self.end_header().hex(' '),
                'automatic_launch': False,
                'limits': 'Host acceptance is not flash readback; installed loader and recovery remain unverified'}


class LegacyTransfer:
    """Requires an already selected, independently validated update-mode target.

    transport.control(type, request, value, index, data_or_length, timeout_ms)
    must return exact OUT byte count or IN bytes, raising on any USB error.
    No live transport is supplied. There are no automatic write retries.
    """
    def __init__(self, transport, *, sleep=time.sleep, polls=100, timeout_ms=1500):
        if polls < 1 or timeout_ms < 1:
            raise ValueError('Positive bounded limits required')
        self.transport = transport
        self.sleep = sleep
        self.polls = polls
        self.timeout_ms = timeout_ms
        self.events = []
        self.used = False

    def out(self, request, payload, value=0):
        result = self.transport.control(0x40, request, value, 0, payload, self.timeout_ms)
        if type(result) is not int or result != len(payload):
            raise TransferError(f'Short/invalid OUT transfer for request {request:02x}; no retry')

    def status(self):
        result = self.transport.control(0xc0, 0xa1, 0, 0, 16, self.timeout_ms)
        if not isinstance(result, (bytes, bytearray)) or len(result) != 16:
            raise TransferError('Short/invalid status reply')
        return bytes(result)

    def wait(self, stage, index=0):
        for _ in range(self.polls):
            reply = self.status()
            code = reply[0]
            echoed = int.from_bytes(reply[1:3], 'little')
            self.events.append({'stage': stage, 'index': index, 'reply': reply.hex(' ')})
            if stage == 'initialize':
                if reply[:5] == b'\x00' * 5:
                    return
                # Unrecognized initialization state is not permission to write.
                raise TransferError('Initialization reply does not match recovered Mac host check')
            if echoed != index:
                raise TransferError(f'Stale/unexpected echoed index {echoed}, expected {index}')
            if stage == 'block' and code == 2:
                return
            if stage == 'end' and code == 4 and reply[3:5] == b'\x00\x00':
                return
            if code != 1:
                raise TransferError(f'Unexpected/rejected {stage} status {code}; no automatic retry')
            self.sleep(0.02)
        raise TransferError(f'Timed out waiting for {stage} acknowledgement')

    def transfer(self, plan):
        if self.used:
            raise TransferError('Engine is single-use; do not restart uncertain transfers automatically')
        self.used = True
        # Validate the entire plan against the official source before the first OUT.
        if not plan.validated():
            raise ValueError('Plan differs from the exact official migration source')
        self.out(0xa1, plan.header(0x33))
        self.sleep(0.01)
        self.wait('initialize')
        for index, payload in enumerate(plan.blocks):
            self.out(0xa2, payload, value=index)
            self.sleep(0.005)
            self.out(0xa1, plan.block_header(index))
            self.sleep(0.1)
            self.wait('block', index)
        self.out(0xa1, plan.end_header())
        self.sleep(0.1)
        self.wait('end', len(plan.blocks))
        return {'status': 'Transfer acknowledged; migration application has NOT been launched',
                'acknowledged_blocks': len(plan.blocks), 'events': list(self.events),
                'verified_readback': False}
