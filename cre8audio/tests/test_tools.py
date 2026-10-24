"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Synthetic fixtures only: no vendor firmware or live hardware access.
"""
from dataclasses import replace
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT / 'tools'))
from firmware_container import Firmware
from hid_protocol import packet, api_buffer
from hid_transfer import VerifiedStagingTransfer, TransferError
from plans import Plan, OFFICIAL_CELLZ
from legacy_transfer import Plan as LegacyPlan, LegacyTransfer


def fixture():
    image = bytearray(b'\x42' * (10*1024))
    struct.pack_into('<II',image,0,0x20001000,0x08003009)
    transport = b'\x23' * 1024
    wrapped = bytes(image[:0x2400]) + transport + bytes(image[0x2400:])
    headers = ['Name:Synthetic test','Firmware Version:V0.0.3.0','VID:1acc',
               'DFU_PID:1a59','APP_PID:1a4d','Tool Version:V0.0.0.2','Count Sum:1',
               f'File1:0/{sum(image)&65535:04x}']
    data = ('\n'.join(headers)+'\n'+wrapped.hex()+'\n').encode()
    plan = Plan(hashlib.sha256(data).hexdigest(),tuple(wrapped[i:i+1024] for i in range(0,len(wrapped),1024)),
                bytes(image),sum(image)&65535)
    return data, plan


class FakeHID:
    def __init__(self,fault=None):
        self.fault, self.commands = fault, []
        self.buffer = bytes(1088)
        self.flash = bytearray(10*1024)

    def set_feature(self,frame):
        self.commands.append(frame[1])
        self.buffer = frame + self.buffer[len(frame):]
        if len(frame) == 1088:
            return True  # Unarmed reception; readback must happen before commit.
        reply = bytearray(self.buffer)
        checksum = reply[63]
        reply[63] = 0
        if sum(reply[:64]) & 255 != checksum:
            raise AssertionError('Bad control frame checksum')
        command = reply[1]
        index = int.from_bytes(reply[10:12],'little')
        if command == 4:
            if index != 9:
                destination = (index if index < 9 else index-1)*1024
                self.flash[destination:destination+1024] = self.buffer[64:]
            if self.fault == 'uncertain_commit':
                raise OSError('Lost commit acknowledgement')
        if command == 5:
            supplied = int.from_bytes(reply[14:16],'little')
            if supplied != sum(self.flash)&65535:
                raise AssertionError('Bad whole-image check')
        reply[2] = 1 if command == 3 else 2
        if command == 5 and self.fault == 'bad_final':
            reply[2] = 3
        self.buffer = bytes(reply)
        return True

    def get_feature(self,report_id=0x60):
        if report_id == 0x70:
            if self.fault == 'corrupt_ram':
                bad = bytearray(self.buffer)
                bad[70] ^= 1
                return bytes(bad)
            return self.buffer
        return self.buffer[:64]


class Tests(unittest.TestCase):
    def test_container_round_trip_and_mutation(self):
        data, plan = fixture()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'synthetic.dfu'
            path.write_bytes(data)
            fw = Firmware.read(path)
            self.assertEqual(fw.checksum,fw.expected_checksum)
            self.assertEqual(fw.repack(fw.image),data)
            changed = bytearray(fw.image)
            changed[10] ^= 1
            path.write_bytes(fw.repack(bytes(changed)))
            rebuilt = Firmware.read(path)
            self.assertEqual(rebuilt.image,bytes(changed))
            self.assertEqual(rebuilt.transport_block,fw.transport_block)
            self.assertEqual(rebuilt.checksum,rebuilt.expected_checksum)

    def test_custom_file_requires_matching_local_manifest(self):
        data, plan = fixture()
        with tempfile.TemporaryDirectory() as directory:
            path, manifest = Path(directory)/'test.dfu', Path(directory)/'build.json'
            path.write_bytes(data)
            with self.assertRaises(ValueError):
                Plan.from_file(path)
            build = {'source_sha256':OFFICIAL_CELLZ,'dfu_sha256':plan.source_sha256,
                     'image_sha256':hashlib.sha256(plan.image).hexdigest(),
                     'checksum':f'{plan.checksum:04x}'}
            manifest.write_text(json.dumps(build))
            self.assertEqual(Plan.from_file(path,manifest),plan)
            build['image_sha256'] = '0'*64
            manifest.write_text(json.dumps(build))
            with self.assertRaises(ValueError):
                Plan.from_file(path,manifest)

    def test_verified_staging_commits_and_checks_expected_image(self):
        _, plan = fixture()
        transport = FakeHID()
        engine = VerifiedStagingTransfer(transport,plan,sleep=lambda _:None)
        result = engine.transfer(plan)
        self.assertEqual(bytes(transport.flash),plan.image)
        self.assertTrue(engine.verified)
        self.assertFalse(result['readback_verified'])
        engine.launch()
        self.assertEqual(transport.commands[-1],1)
        with self.assertRaises(TransferError):
            engine.launch()

    def test_faults_do_not_retry_or_launch(self):
        _, plan = fixture()
        for fault in ('corrupt_ram','uncertain_commit','bad_final'):
            with self.subTest(fault=fault):
                transport = FakeHID(fault)
                engine = VerifiedStagingTransfer(transport,plan,sleep=lambda _:None)
                with self.assertRaises((OSError,TransferError)):
                    engine.transfer(plan)
                with self.assertRaises(TransferError):
                    engine.launch()
                self.assertNotIn(1,transport.commands)
                if fault == 'corrupt_ram':
                    self.assertNotIn(4,transport.commands)
                if fault == 'uncertain_commit':
                    self.assertEqual(transport.commands.count(4),1)

    def test_changed_plan_stops_before_io(self):
        _, plan = fixture()
        transport = FakeHID()
        engine = VerifiedStagingTransfer(transport,plan)
        with self.assertRaises(ValueError):
            engine.transfer(replace(plan,checksum=0))
        self.assertEqual(transport.commands,[])

    def test_legacy_migration_rejects_unknown_payload_before_io(self):
        plan = LegacyPlan('0'*64,(b'\0'*1024,)*16)
        class NoUSB:
            def control(self,*args):
                raise AssertionError('Unexpected USB request')
        with self.assertRaises(ValueError):
            LegacyTransfer(NoUSB()).transfer(plan)

    def test_cli_help_requires_no_usb_or_vendor_files(self):
        for script in ('cellz_update.py','build_harmony.py'):
            result = subprocess.run([sys.executable,str(ROOT/'tools'/script),'--help'],
                                    capture_output=True,text=True,check=True)
            self.assertIn('Maarten Hofman (cashimor) is not responsible',result.stdout)

    def test_short_and_long_report_api_lengths(self):
        self.assertEqual(len(api_buffer(packet(2,count=11))),1088)
        self.assertEqual(len(api_buffer(packet(3,count=11,payload=b'\0'*1024))),1088)


if __name__ == '__main__':
    unittest.main()
