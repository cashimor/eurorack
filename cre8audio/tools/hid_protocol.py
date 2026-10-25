# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk.
"""Feature report framing recovered from the supplied HID loader and host."""
import struct


def drum_status(report):
    """Decode the read-only Drum extension; other applications return None."""
    if len(report) != 64 or report[16:24] not in (b'DRUM0001', b'DRUM0002', b'DRUM0003'):
        return None
    instrument, selected, active, step, external, dirty, saved = report[24:31]
    d3 = report[16:24] == b'DRUM0003'
    length, page, pickup = report[31:34] if d3 else (16, 0, 0)
    if length not in (16, 32) or page >= length // 16 or pickup > 1:
        raise ValueError('Invalid Drum length/page fields')
    if instrument > 3 or selected > 1 or active > 1 or step not in (*range(length), 255) or external > 1 or dirty > 1 or saved > 2:
        raise ValueError('Invalid Drum status fields')
    status = {'instrument': instrument + 1, 'selected_bank': 'AB'[selected],
            'playing_and_editing_bank': 'AB'[active],
            'step': None if step == 255 else step + 1,
            'external_clock': bool(external), 'unsaved_edits': bool(dirty),
            'last_save': ('none', 'success', 'failed')[saved]}
    if d3:
        status.update(length_steps=length, selected_page=f"{'AB'[selected]}{page+1}",
                      playing_and_editing_page=f"{'AB'[active]}{page+1}",
                      swing_pickup=bool(pickup))
    return status


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
