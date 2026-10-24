"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Windows Cellz transports. No driver changes, firmware files, or USB writes on import.
"""
import hashlib
import os
import time
from hid_protocol import packet
from windows_hid import Native, WindowsHid

# Fingerprint of the inspected 49-byte HID descriptor, not a bundled vendor blob.
REPORT_HASH = '7e0fb12da9b74fd95f7127fc146850aee99f52bc1895ba08bd94ff605a607492'


def close(device):
    import usb.util
    usb.util.dispose_resources(device)


def location(device):
    ports = tuple(device.port_numbers or ())
    if device.bus is None or not ports:
        raise RuntimeError('Cannot establish physical USB location')
    return device.bus, ports


def describe(device):
    import usb.util
    return {'vid': f'{device.idVendor:04x}', 'pid': f'{device.idProduct:04x}',
            'revision': f'{device.bcdDevice:04x}',
            'product': usb.util.get_string(device, device.iProduct),
            'location': location(device)}


class RawFeature:
    def __init__(self, device, interface, write=False):
        self.device, self.interface, self.write = device, interface, write
        descriptor = bytes(device.ctrl_transfer(0x81,6,0x2200,interface,49,timeout=1500))
        if hashlib.sha256(descriptor).hexdigest() != REPORT_HASH:
            raise ValueError('Unexpected HID descriptor; refusing feature access')

    def set_feature(self, frame):
        from hid_protocol import api_buffer
        if not self.write:
            raise PermissionError('Feature writes disabled')
        api_buffer(frame)
        count = self.device.ctrl_transfer(0x21,9,0x300|frame[0],self.interface,frame,timeout=1500)
        if count != len(frame):
            raise OSError('Short feature write; outcome uncertain')
        return True

    def get_feature(self, report_id=0x60):
        if report_id not in (0x60,0x70):
            raise ValueError('Unsupported feature report')
        size = 64 if report_id == 0x60 else 1088
        result = bytes(self.device.ctrl_transfer(0xa1,1,0x300|report_id,self.interface,size,timeout=1500))
        if len(result) != size:
            raise OSError('Short feature read')
        return result

    def close(self):
        close(self.device)


class LegacyUSB:
    def __init__(self, device):
        self.device = device

    def control(self, typ, request, value, index, data, timeout):
        allowed = (typ == 0xc0 and request == 0xa1 and value == index == 0 and data == 16)
        allowed |= (typ == 0x40 and index == 0 and isinstance(data,bytes) and (
            (request == 0xa1 and value == 0 and len(data) == 16 and data[0] in (0x32,0x33,0x34))
            or (request == 0xa2 and 0 <= value < 16 and len(data) == 1024)))
        if not allowed:
            raise ValueError('Request outside reviewed migration protocol')
        result = self.device.ctrl_transfer(typ,request,value,index,data,timeout=timeout)
        return bytes(result) if typ & 128 else result

    def close(self):
        close(self.device)


class Cellz:
    def __init__(self, write=False):
        if os.name != 'nt':
            raise RuntimeError('Live USB operations currently support Windows only')
        self.write = write
        devices = self.devices()
        if len(devices) != 1:
            raise RuntimeError('Connect exactly one Cre8audio USB device')
        self.device = devices[0]
        self.location = location(self.device)
        self.identity = describe(self.device)
        close(self.device)  # Do not retain an initial WinUSB handle across enumeration.

    @staticmethod
    def devices():
        import libusb_package
        return list(libusb_package.find(find_all=True,idVendor=0x1acc))

    def _one(self):
        devices = self.devices()
        if len(devices) > 1:
            raise RuntimeError('Ambiguous Cre8audio devices; disconnect the others')
        if not devices:
            return None
        if location(devices[0]) != self.location:
            raise RuntimeError('Device appeared at a different USB location')
        return devices[0]

    def wait(self, predicate, seconds=30):
        deadline = time.monotonic() + seconds
        last_error = None
        while time.monotonic() < deadline:
            time.sleep(0.25)
            device = self._one()
            if device is None:
                continue
            try:
                identity = describe(device)
                if predicate(device, identity):
                    self.device, self.identity = device, identity
                    return identity
            except (OSError,NotImplementedError,ValueError) as exc:
                last_error = str(exc)
            finally:
                close(device)
        raise TimeoutError(f'Expected USB state did not appear: {last_error}')

    def features(self, write=False, device=None):
        if write and not self.write:
            raise PermissionError('Live writes disabled')
        device = device or self._one()
        if device is None:
            raise RuntimeError('Cellz disconnected')
        candidates = [x for x in Native().enumerate() if x['pid'] == device.idProduct]
        if len(candidates) > 1:
            raise RuntimeError('Ambiguous native HID collections')
        if len(candidates) == 1:
            close(device)
            return WindowsHid(candidates[0],allow_write=write)
        config = device.get_active_configuration()
        interfaces = [i for i in config if i.bInterfaceClass == 3]
        if len(interfaces) != 1:
            raise RuntimeError('Expected one HID interface')
        return RawFeature(device,interfaces[0].bInterfaceNumber,write)

    def report(self, device=None):
        transport = self.features(device=device)
        try:
            return transport.get_feature(0x60)[:64]
        finally:
            transport.close()

    @staticmethod
    def info(device):
        result = bytes(device.ctrl_transfer(0xc0,0x11,0,0,16,timeout=1500))
        if len(result) != 16:
            raise OSError('Short legacy identification')
        return result

    def inspect(self):
        result = {'identity': self.identity}
        try:
            result['hid_status'] = self.report().hex(' ')
        except (OSError,NotImplementedError,ValueError,RuntimeError) as exc:
            result['hid_status_error'] = str(exc)
            device = self._one()
            try:
                result['legacy_information'] = self.info(device).hex(' ')
            except (OSError,NotImplementedError) as exc:
                result['legacy_information_error'] = str(exc)
            finally:
                if device is not None:
                    close(device)
        return result

    def enter_hid(self):
        if not self.write:
            raise PermissionError('Live writes disabled')
        if self.identity['product'] == 'Cre8audioCellz':
            report = self.report()
            if report[:6] != bytes.fromhex('600102400001') or report[8:10] != b'\x30\x00':
                raise RuntimeError('Software entry supports the inspected V0.0.3.0 app only; use the recovery switch')
            transport = self.features(write=True)
            try:
                transport.set_feature(packet(1,parameter=0))
            except OSError:
                pass  # Reset may interrupt completion; never resend automatically.
            finally:
                transport.close()
        elif self.identity['product'] != 'Nifty Series Upgrade':
            raise RuntimeError('Enter the new HID loader with the recovery switch first')

        def ready(device, identity):
            return (identity['product'] == 'Nifty Series Upgrade' and identity['revision'] == '0102'
                    and self.report(device)[:8] == bytes.fromhex('6001024000000201'))
        return self.wait(ready)

    def migrate(self, plan, engine):
        if not self.write or self.identity['product'] != 'Cre8audioCellz':
            raise RuntimeError('Migration must start from an identifiable Cellz application')
        device = self._one()
        try:
            if self.info(device)[8:11] != b'\x1b\x00\x00':
                raise RuntimeError('Migration is limited to the tested legacy Cellz revision 001b')
            try:
                device.ctrl_transfer(0x40,0x10,0,1,b'',timeout=1500)
            except OSError:
                pass  # Identify the resulting mode; do not repeat the request.
        finally:
            close(device)

        def legacy(device, identity):
            return (identity['product'] == 'Nifty Series'
                    and self.info(device)[8:11] == b'\x03\x00\x01')
        self.wait(legacy)
        transport = LegacyUSB(self._one())
        try:
            result = engine(transport).transfer(plan)
        finally:
            transport.close()
        device = self._one()
        try:
            device.ctrl_transfer(0x40,0x10,0,0,b'',timeout=1500)
        except OSError:
            pass
        finally:
            close(device)

        def installer(device, identity):
            return (identity['product'] == 'Cre8audio boot updata'
                    and self.info(device)[8:11] == b'\x01\x00\x00')
        result['installer_identity'] = self.wait(installer)
        result['bootloader_readback_verified'] = False
        result['next_step'] = 'Use the switch beside USB to enter the HID loader, then run flash. See docs/windows-updater.md.'
        return result

    def application(self, signature=None):
        def ready(device, identity):
            if identity['product'] != 'Cre8audioCellz':
                return False
            report = self.report(device)
            if report[:6] != bytes.fromhex('600102400001') or report[8:10] != b'\x30\x00':
                return False
            if signature is not None and report[16:24] != signature:
                return False
            self.application_report = report
            return True
        identity = self.wait(ready)
        return {'identity': identity, 'hid_status': self.application_report.hex(' '),
                'harmony': self.application_report[16:24] in (b'HARMONY1', b'HARMONY2')}
