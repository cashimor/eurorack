# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk.
"""Feature report framing recovered from the supplied HID loader and host."""
import struct


def packet(command, index=0, count=0, checksum=0, payload=b'', parameter=0):
    if command not in range(1, 6) or not all(0 <= x <= 65535 for x in (index, count, checksum)):
        raise ValueError('Unsupported command or field range')
    if payload and (command != 3 or len(payload) != 1024):
        raise ValueError('Only command 3 carries a 1024-byte payload')
    if command == 3 and len(payload) != 1024:
        raise ValueError('Staging requires a full block')
    if parameter not in (0, 1) or (parameter and command != 1):
        raise ValueError('Unsupported mode parameter')
    data = bytearray(64 + len(payload))
    data[0:6] = bytes([0x70 if payload else 0x60, command, 0x0f, 0x40,
                       4 if payload else 0, parameter])
    data[16:63] = b'\x0a' * 47
    struct.pack_into('<HHH', data, 10, index, count, checksum)
    data[64:] = payload
    data[63] = sum(data) & 0xff
    return bytes(data)


def api_buffer(frame, maximum=1088):
    """Match UsbHid.dll's maximum-capability buffer, including report ID."""
    if len(frame) not in (64, 1088) or maximum != 1088 or frame[0] not in (0x60, 0x70):
        raise ValueError('Unexpected feature report framing/capability')
    if int.from_bytes(frame[3:5], 'little') != len(frame):
        raise ValueError('Declared report length mismatch')
    return frame.ljust(maximum, b'\x00')
