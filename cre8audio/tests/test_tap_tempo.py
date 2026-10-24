"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Pure clock scenarios; no USB or vendor code. Also reused by private ARM tests.
"""
import ctypes
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
COMPILER = shutil.which('cc') or shutil.which('gcc')


class Voice(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint32) for name in
                ('period_us', 'phase_us', 'changed_ms', 'pressed_ms', 'last_tap_ms')]
    _fields_ += [('intervals', ctypes.c_uint32 * 4)]
    _fields_ += [(name, ctypes.c_uint8) for name in
                ('raw', 'down', 'held', 'taps', 'count', 'cursor')]


class State(ctypes.Structure):
    _fields_ = [('last_ms', ctypes.c_uint32), ('voice', Voice * 2)]


class Harness:
    def __init__(self, lib, now=0):
        self.lib, self.state, self.now, self.mask = lib, State(), now, 0
        self.lib.harmony_tempo_init(ctypes.byref(self.state), now)

    def update(self, now, mask=None):
        self.now = now & 0xffffffff
        if mask is not None:
            self.mask = mask
        return self.lib.harmony_tempo_update(ctypes.byref(self.state), self.now, self.mask)

    def period(self, voice):
        return self.state.voice[voice].period_us

    def set_period(self, voice, period_us):
        self.state.voice[voice].period_us = period_us

    def tap(self, at, mask=1):
        self.update(at, mask)
        self.update(at + 25)
        self.update(at + 50, 0)
        return self.update(at + 75)

    def train(self, channel, interval, start=100, count=3):
        return [self.tap(start + i * interval, 1 << channel) for i in range(count)]


class Scenarios:
    def test_three_taps_and_rolling_average(self):
        h = self.harness()
        self.assertEqual(h.tap(100), 0)
        self.assertEqual(h.tap(600), 0)
        self.assertEqual(h.period(0), 0)
        self.assertEqual(h.tap(1100), 0)
        self.assertEqual(h.period(0), 500000)
        h.tap(1620)
        self.assertEqual(h.period(0), 506667)
        h.tap(2100)
        self.assertEqual(h.period(0), 500000)

    def test_ratio_examples_and_extended_eight_to_one(self):
        for bpm, expected in ((120, 500000), (60, 1000000), (40, 1500000),
                              (30, 2000000), (35, 1500000), (15, 4000000)):
            h = self.harness()
            h.train(0, 500)
            h.train(1, round(60000/bpm), start=2000)
            self.assertEqual(h.period(0), 500000)
            self.assertEqual(h.period(1), expected)
        h = self.harness()
        h.train(0, 4000)
        h.train(1, 500, start=9000)
        self.assertEqual((h.period(0), h.period(1)), (4000000, 500000))

    def test_hold_stops_only_that_direction_and_release_does_not_restart(self):
        h = self.harness()
        h.train(0, 500)
        h.train(1, 1000, start=2000)
        h.update(4500, 1)
        h.update(4525)
        h.update(5299)
        self.assertNotEqual(h.period(0), 0)
        h.update(5300)
        self.assertEqual((h.period(0), h.period(1)), (0, 1000000))
        h.update(5400, 0)
        h.update(5425)
        self.assertEqual(h.period(0), 0)
        h.tap(5500)
        h.tap(6000)
        self.assertEqual(h.period(0), 0)
        h.tap(6500)
        self.assertEqual(h.period(0), 500000)

    def test_both_buttons_are_independent_and_never_a_special_gesture(self):
        h = self.harness()
        for when in (100, 600, 1100):
            result = h.tap(when, 3)
        self.assertEqual(result, 0)
        self.assertEqual((h.period(0), h.period(1)), (500000, 500000))
        h.update(2000, 3)
        h.update(2025)
        h.update(2800)
        self.assertEqual((h.period(0), h.period(1)), (0, 0))

    def test_bounce_and_long_pause_do_not_supply_extra_taps(self):
        h = self.harness()
        for at, mask in ((100, 1), (105, 0), (110, 1), (115, 0), (140, 0)):
            h.update(at, mask)
        self.assertEqual(h.period(0), 0)
        h.tap(200)
        h.tap(700)
        h.tap(6000)
        self.assertEqual(h.period(0), 0)
        h.tap(6500)
        self.assertEqual(h.period(0), 0)
        h.tap(7000)
        self.assertEqual(h.period(0), 500000)

    def test_release_just_before_hold_threshold_is_a_tap(self):
        h = self.harness()
        h.train(0, 1000)
        h.update(3100, 1)
        h.update(3125)
        h.update(3890, 0)
        h.update(3905)
        h.update(3910)
        self.assertEqual(h.period(0), 1000000)

    def test_irregular_taps_require_a_new_confirming_interval(self):
        h = self.harness()
        h.tap(100)
        h.tap(600)
        h.tap(1600)
        self.assertEqual(h.period(0), 0)
        h.tap(2600)
        self.assertEqual(h.period(0), 1000000)

    def test_wraparound_and_no_catch_up_burst(self):
        h = self.harness(now=0xfffffa00)
        h.train(0, 500, start=0xfffffa10)
        self.assertEqual(h.period(0), 500000)
        self.assertEqual(h.update(h.now + 10000) & 1, 1)
        self.assertEqual(h.update(h.now), 0)
        self.assertEqual(h.update(h.now + 499), 0)
        self.assertEqual(h.update(h.now + 1) & 1, 1)

    def test_fractional_period_has_no_millisecond_rounding_drift(self):
        h = self.harness()
        h.train(0, 500)
        h.train(1, 333, start=2000)
        self.assertEqual(h.period(1), 333333)
        events = [0, 0]
        start = h.now
        for elapsed in range(1, 10001):
            due = h.update(start + elapsed)
            for channel in range(2):
                events[channel] += bool(due & (1 << channel))
        self.assertEqual(events, [20, 30])

    def test_eight_to_one_clock_waits_full_period_across_sparse_updates(self):
        h = self.harness()
        h.train(0, 4000)
        # Establish the slow anchor, then select an eightfold slower duration
        # directly to exercise the full supported scheduler range.
        h.set_period(1, 32000000)
        start = h.now
        self.assertEqual(h.update(start+10000) & 2, 0)
        self.assertEqual(h.update(start+20000) & 2, 0)
        self.assertEqual(h.update(start+31999) & 2, 0)
        self.assertEqual(h.update(start+32000) & 2, 2)

    def test_setting_either_direction_aligns_both_running_cycles(self):
        for selected in (0, 1):
            h = self.harness()
            h.train(selected ^ 1, 500)
            h.train(selected, 2000, start=2000)
            start = h.now
            fast, slow = 1 << (selected ^ 1), 1 << selected
            self.assertEqual(h.period(selected), 2000000)
            self.assertEqual(h.update(start + 499), 0)
            self.assertEqual(h.update(start + 500), fast)
            self.assertEqual(h.update(start + 1000), fast)
            self.assertEqual(h.update(start + 1500), fast)
            self.assertEqual(h.update(start + 1999), 0)
            self.assertEqual(h.update(start + 2000), fast | slow)

    def test_confirmed_retap_near_a_clock_edge_has_no_extra_step(self):
        h = self.harness()
        h.train(0, 500)
        # Last scheduled edge at 1675; the next accepted release arrives at
        # 1695, only 20 ms later. It must reset time without another jump.
        self.assertEqual(h.update(1620, 1), 0)
        self.assertEqual(h.update(1645), 0)
        self.assertEqual(h.update(1670, 0), 0)
        self.assertEqual(h.update(1675), 1)
        self.assertEqual(h.update(1695), 0)
        self.assertEqual(h.period(0), 506667)
        self.assertEqual(h.update(1695), 0)
        self.assertEqual(h.update(2201), 0)
        self.assertEqual(h.update(2202), 1)

    def test_tapping_on_each_due_edge_keeps_playing_without_duplicate_steps(self):
        h = self.harness()
        h.train(0, 500)
        for at in (1600,2100,2600,3100):
            self.assertEqual(h.tap(at), 1)
            self.assertEqual(h.update(h.now), 0)

    def test_a_hold_stop_suppresses_an_edge_due_on_the_same_update(self):
        h = self.harness()
        h.train(0, 500)
        h.update(1375,1)
        h.update(1400)
        self.assertEqual(h.update(2175),0)
        self.assertEqual(h.period(0),0)


@unittest.skipUnless(COMPILER and sys.platform != 'win32', 'Native C tests use a Unix C compiler')
class NativeTests(Scenarios, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        library = Path(cls.directory.name) / 'tap.so'
        subprocess.run([COMPILER, '-shared', '-fPIC', '-std=c11',
                        str(ROOT/'firmware/tap_tempo.c'), '-o', str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.harmony_tempo_init.argtypes = [ctypes.POINTER(State), ctypes.c_uint32]
        cls.lib.harmony_tempo_update.argtypes = [ctypes.POINTER(State), ctypes.c_uint32, ctypes.c_uint32]
        cls.lib.harmony_tempo_update.restype = ctypes.c_uint32

    @classmethod
    def tearDownClass(cls):
        cls.lib = None
        cls.directory.cleanup()

    def harness(self, now=0):
        return Harness(self.lib, now)
