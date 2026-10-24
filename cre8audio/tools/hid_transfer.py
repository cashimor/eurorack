# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use. Use at your own risk.
"""Reviewed Cellz application transfer; injected transport; no writes on import."""
import time
from hid_protocol import packet


class TransferError(RuntimeError):
    pass


class HidTransfer:
    """Transport.set_feature(frame) returns True; get_feature(0x60) returns >=64 bytes.

    Control responses echo the command/index/count, but their checksum field
    is cleared by the actual handler. Do not invent a response CRC check.
    """
    def __init__(self, transport, reviewed_plan, *, sleep=time.sleep, polls=100):
        if polls < 1:
            raise ValueError('Bounded positive polling limit required')
        self.transport, self.sleep, self.polls = transport, sleep, polls
        self.reviewed_plan = reviewed_plan
        self.used = False
        self.verified = False
        self.launched = False
        self.events = []

    def exchange(self, frame, expected):
        if self.transport.set_feature(frame) is not True:
            raise TransferError('HID write failed or uncertain; no automatic retry')
        self.sleep(0.03)
        for _ in range(self.polls):
            raw = self.transport.get_feature(0x60)
            if not isinstance(raw, bytes) or len(raw) < 64:
                raise TransferError('Short/invalid HID status response')
            reply = raw[:64]
            # Staging responds from the same buffer with ID 70, even though
            # status is requested through report 60. Accept its expected echo.
            if reply[0] != frame[0] or reply[1] != frame[1] or reply[3:5] != frame[3:5] or reply[10:14] != frame[10:14]:
                raise TransferError('Stale or mismatched HID response')
            status = reply[2]
            self.events.append({'command': frame[1], 'index': int.from_bytes(frame[10:12], 'little'), 'status': status})
            if status == expected:
                if frame[1] == 5 and reply[14:16] != frame[14:16]:
                    raise TransferError('Final checksum echo differs')
                return reply
            # Status 1 is final for staging and pending for the other commands.
            if status != 1 or expected == 1:
                raise TransferError(f'Rejected HID command {frame[1]}, status {status}')
            self.sleep(0.02)
        raise TransferError('Timed out waiting for HID status')

    def transfer(self, plan):
        if self.used:
            raise TransferError('Single-use transfer; uncertain commits must not be replayed automatically')
        self.used = True
        self.validate_plan(plan)
        count = len(plan.blocks)
        self.exchange(packet(2, count=count), 2)
        for index, block in enumerate(plan.blocks):
            self.stage(index, count, block)
            self.exchange(packet(4, index=index, count=count), 2)
        self.exchange(packet(5, index=count, count=count, checksum=plan.checksum), 2)
        self.verified = True
        return {'status': 'Whole-image checksum accepted; application has not been launched',
                'checksum': f'{plan.checksum:04x}', 'blocks': count, 'events': list(self.events),
                'readback_verified': False}

    def validate_plan(self, plan):
        if plan != self.reviewed_plan:
            raise ValueError('Altered application plan')
        plan.validate()

    def stage(self, index, count, block):
        self.exchange(packet(3, index=index, count=count, payload=block), 1)

    def launch(self):
        if not self.verified or self.launched:
            raise TransferError('Return requires successful checksum and can only be requested once')
        self.launched = True
        # The host sends this without GET: it schedules a timed application jump.
        if self.transport.set_feature(packet(1, parameter=1)) is not True:
            raise TransferError('Application-return request failed or uncertain; do not retry automatically')


class VerifiedStagingTransfer(HidTransfer):
    """Disarm partial USB reception, verify RAM, then arm a short stage header.

    Command 3 has no dispatch action beyond packet validation/status 1. Its
    payload remains in the shared RAM buffer for the separate commit command.
    The 64-byte header preserves those payload bytes. Flash writes and final
    image checksum follow the same sequence as HidTransfer.
    """
    def stage(self, index, count, block):
        unarmed = bytearray(packet(3, index=index, count=count, payload=block))
        unarmed[2] = 0  # Main handler only consumes request tag 0x0f.
        unarmed[63] = 0
        unarmed[63] = sum(unarmed) & 255
        if self.transport.set_feature(bytes(unarmed)) is not True:
            raise TransferError('RAM staging write uncertain; no retry')
        received = self.transport.get_feature(0x70)
        if received != bytes(unarmed):
            raise TransferError('Full RAM staging readback differs; no flash commit')
        self.events.append({'command': 3, 'index': index, 'ram_readback_verified': True})
        # Construct an ordinary 64-byte control frame with command 3. The
        # generic packet builder deliberately requires payload for its regular
        # command-3 path, so derive the header from the same-index commit frame.
        arm = bytearray(packet(4, index=index, count=count))
        arm[1], arm[63] = 3, 0
        arm[63] = sum(arm) & 255
        self.exchange(bytes(arm), 1)
