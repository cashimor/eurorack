# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk.
"""Windows native feature-report transport. No writes on import/enumeration.

Blocking HidD feature calls run in one disposable worker process. A timeout
terminates it and makes the session unusable; uncertain writes are not retried.
"""
import ctypes as c
from ctypes import wintypes as w
import multiprocessing as mp
import re
from hid_types import GUID, Interface, Attributes, Caps
from hid_protocol import api_buffer


class Native:
    def __init__(self):
        self.hid = c.WinDLL('hid', use_last_error=True)
        self.setup = c.WinDLL('setupapi', use_last_error=True)
        self.kernel = c.WinDLL('kernel32', use_last_error=True)
        ptr = c.c_void_p
        self.setup.SetupDiGetClassDevsW.argtypes = [ptr, w.LPCWSTR, ptr, w.DWORD]
        self.setup.SetupDiGetClassDevsW.restype = ptr
        self.setup.SetupDiEnumDeviceInterfaces.argtypes = [ptr, ptr, ptr, w.DWORD, ptr]
        self.setup.SetupDiGetDeviceInterfaceDetailW.argtypes = [ptr, ptr, ptr, w.DWORD, ptr, ptr]
        self.setup.SetupDiDestroyDeviceInfoList.argtypes = [ptr]
        self.kernel.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, ptr, w.DWORD, w.DWORD, ptr]
        self.kernel.CreateFileW.restype = ptr
        self.kernel.CloseHandle.argtypes = [ptr]
        self.hid.HidD_GetHidGuid.argtypes = [ptr]
        for name in ('HidD_GetAttributes', 'HidD_GetPreparsedData'):
            getattr(self.hid, name).argtypes = [ptr, ptr]
            getattr(self.hid, name).restype = c.c_ubyte
        self.hid.HidD_FreePreparsedData.argtypes = [ptr]
        self.hid.HidP_GetCaps.argtypes = [ptr, ptr]
        self.hid.HidP_GetCaps.restype = c.c_long
        for name in ('HidD_GetProductString', 'HidD_GetSerialNumberString', 'HidD_GetFeature', 'HidD_SetFeature'):
            getattr(self.hid, name).argtypes = [ptr, ptr, w.ULONG]
            getattr(self.hid, name).restype = c.c_ubyte

    def open(self, path, access=0):
        handle = self.kernel.CreateFileW(path, access, 3, None, 3, 0, None)
        if handle == c.c_void_p(-1).value:
            raise c.WinError(c.get_last_error())
        return handle

    def enumerate(self):
        guid = GUID()
        self.hid.HidD_GetHidGuid(c.byref(guid))
        devs = self.setup.SetupDiGetClassDevsW(c.byref(guid), None, None, 0x12)
        if devs == c.c_void_p(-1).value:
            raise c.WinError(c.get_last_error())
        result = []
        try:
            index = 0
            while True:
                interface = Interface()
                interface.size = c.sizeof(interface)
                if not self.setup.SetupDiEnumDeviceInterfaces(devs, None, c.byref(guid), index, c.byref(interface)):
                    if c.get_last_error() == 259:
                        break
                    raise c.WinError(c.get_last_error())
                index += 1
                needed = w.DWORD()
                self.setup.SetupDiGetDeviceInterfaceDetailW(devs, c.byref(interface), None, 0, c.byref(needed), None)
                detail = c.create_string_buffer(needed.value)
                c.cast(detail, c.POINTER(w.DWORD))[0] = 8 if c.sizeof(c.c_void_p) == 8 else 6
                if not self.setup.SetupDiGetDeviceInterfaceDetailW(devs, c.byref(interface), detail, needed, None, None):
                    raise c.WinError(c.get_last_error())
                path = c.wstring_at(c.addressof(detail) + 4)
                if not re.search(r'vid_1acc&pid_(1a4d|1a59)', path.lower()):
                    continue
                handle = self.open(path)
                try:
                    attrs = Attributes()
                    attrs.size = c.sizeof(attrs)
                    if not self.hid.HidD_GetAttributes(handle, c.byref(attrs)):
                        raise c.WinError(c.get_last_error())
                    item = {'path': path, 'vid': attrs.vid, 'pid': attrs.pid, 'revision': attrs.version}
                    for label, method in [('product', 'HidD_GetProductString'), ('serial', 'HidD_GetSerialNumberString')]:
                        buf = c.create_unicode_buffer(128)
                        if not getattr(self.hid, method)(handle, buf, c.sizeof(buf)):
                            raise c.WinError(c.get_last_error())
                        item[label] = buf.value
                    prep = c.c_void_p()
                    if not self.hid.HidD_GetPreparsedData(handle, c.byref(prep)):
                        raise c.WinError(c.get_last_error())
                    try:
                        caps = Caps()
                        if self.hid.HidP_GetCaps(prep, c.byref(caps)) != 0x110000:
                            raise RuntimeError('HID capability parse failed')
                        item.update(feature_length=caps.feature, usage_page=caps.page, usage=caps.usage)
                    finally:
                        self.hid.HidD_FreePreparsedData(prep)
                    result.append(item)
                finally:
                    self.kernel.CloseHandle(handle)
        finally:
            self.setup.SetupDiDestroyDeviceInfoList(devs)
        return result


def _worker(path, allow_write, pipe):
    handle = None
    try:
        api = Native()
        handle = api.open(path, 0xc0000000 if allow_write else 0)
        pipe.send(('ready', None))
        while True:
            op, data = pipe.recv()
            if op == 'close':
                return
            if op == 'set':
                if not allow_write:
                    raise PermissionError('Read-only HID session')
                buf = c.create_string_buffer(api_buffer(data), 1088)
                if not api.hid.HidD_SetFeature(handle, buf, 1088):
                    raise c.WinError(c.get_last_error())
                pipe.send(('ok', True))
            elif op == 'get' and data in (0x60, 0x70):
                buf = c.create_string_buffer(bytes([data]) + b'\xff' * 1087, 1088)
                if not api.hid.HidD_GetFeature(handle, buf, 1088):
                    raise c.WinError(c.get_last_error())
                pipe.send(('ok', bytes(buf)))
            else:
                raise ValueError('Unsupported HID operation')
    except Exception as exc:
        try:
            pipe.send(('error', str(exc)))
        except (BrokenPipeError, EOFError):
            pass
    finally:
        if handle is not None:
            api.kernel.CloseHandle(handle)
        pipe.close()


class WindowsHid:
    def __init__(self, candidate, *, allow_write=False, timeout=5):
        if candidate['vid'] != 0x1acc or candidate['pid'] not in (0x1a4d, 0x1a59) or candidate['feature_length'] != 1088 or candidate['usage_page'] != 0xff01:
            raise ValueError('Unexpected loader HID identity/capabilities')
        if not 0 < timeout <= 30:
            raise ValueError('Invalid HID timeout')
        self.allow_write, self.timeout, self.closed = allow_write, timeout, False
        ctx = mp.get_context('spawn')
        self.pipe, child = ctx.Pipe()
        self.process = ctx.Process(target=_worker, args=(candidate['path'], allow_write, child), daemon=True)
        self.process.start()
        child.close()
        if self._receive()[0] != 'ready':
            self.close()
            raise RuntimeError('Could not open HID worker')

    def _receive(self):
        if not self.pipe.poll(self.timeout):
            self.close()
            raise TimeoutError('HID I/O timed out; session terminated, write outcome may be uncertain')
        try:
            kind, value = self.pipe.recv()
        except EOFError:
            self.close()
            raise OSError('HID worker exited; outcome may be uncertain')
        if kind == 'error':
            self.close()
            raise OSError(value)
        return kind, value

    def _call(self, op, value):
        if self.closed:
            raise RuntimeError('HID session closed')
        self.pipe.send((op, value))
        return self._receive()[1]

    def set_feature(self, frame):
        if not self.allow_write:
            raise PermissionError('HID writes disabled')
        api_buffer(frame)  # Validate before worker I/O.
        return self._call('set', frame)

    def get_feature(self, report_id=0x60):
        if report_id not in (0x60, 0x70):
            raise ValueError('Only status report 60 and staging report 70 are supported')
        return self._call('get', report_id)

    def close(self):
        if self.closed:
            return
        self.closed = True
        if self.process.is_alive():
            self.process.terminate()
        self.process.join(timeout=2)
        self.pipe.close()
