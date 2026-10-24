"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Compile our pure C note generator on the host. No vendor image is required.
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

# Independent expected Roman-numeral roots/qualities. Minor-home progressions
# use the natural-minor III/VI/VII; bII and a major tonic are explicit borrowings.
PROGRESSIONS = [
    ((0,0),(7,0),(9,1),(5,0)), ((2,1),(7,0),(0,0),(0,0)),
    ((0,0),(0,0),(5,0),(7,0)), ((0,0),(9,1),(5,0),(7,0)),
    ((9,1),(5,0),(0,0),(7,0)), ((0,0),(9,1),(2,1),(7,0)),
    ((5,0),(7,0),(0,0),(9,1)), ((0,0),(4,0),(5,0),(5,1)),
    ((0,0),(10,0),(5,0),(0,0)), ((0,1),(10,0),(8,0),(7,0)),
    ((0,1),(5,1),(7,1),(0,1)), ((0,1),(8,0),(3,0),(10,0)),
    ((5,1),(3,0),(1,0),(0,0)),
]
MAJOR, MINOR = (0,2,4,5,7,9,11), (0,2,3,5,7,8,10)


class ProgressionScenarios:
    def test_all_progression_maps_have_correct_chords_scales_and_unique_pairs(self):
        for progression, chords in enumerate(PROGRESSIONS):
            arrangements = set()
            coverage = [set() for _ in range(4)]
            for layout in range(16):
                low, high = self.generate(14+progression,layout)
                self.assertEqual(len(set(zip(low,high))),16)
                self.assertTrue(all(0 <= value <= 24 for value in (*low,*high)))
                arrangements.add((tuple(low),tuple(high)))
                for pad in range(16):
                    source = ((15-pad)*(1,5,11,15)[layout//4]+4*(layout%4))%16
                    chord = source//4
                    root, minor = chords[chord]
                    self.assertIn(low[pad]%12,(root,(root+(3 if minor else 4))%12,(root+7)%12))
                    scale = MINOR if minor else MAJOR
                    self.assertIn((high[pad]-root)%12,scale)
                    coverage[chord].add((high[pad]-root)%12)
            self.assertEqual(len(arrangements),16)
            for chord, (_,minor) in enumerate(chords):
                self.assertEqual(coverage[chord],set(MINOR if minor else MAJOR))

    def test_base_layout_has_four_contiguous_chord_groups(self):
        for progression, chords in enumerate(PROGRESSIONS):
            low, _ = self.generate(14+progression,0)
            for chord, (root,minor) in enumerate(chords):
                self.assertEqual(tuple(low[15-chord*4-slot] for slot in range(4)),
                                 (root,root+(3 if minor else 4),root+7,root+12))

    def test_melody_uses_scale_tones_beyond_the_triad(self):
        for progression,chords in enumerate(PROGRESSIONS):
            for chord,(root,minor) in enumerate(chords):
                notes = set()
                for layout in range(16):
                    _, high = self.generate(14+progression,layout)
                    for pad in range(16):
                        source = ((15-pad)*(1,5,11,15)[layout//4]+4*(layout%4))%16
                        if source//4 == chord:
                            notes.add((high[pad]-root)%12)
                self.assertTrue(notes - {0,3 if minor else 4,7})

    def test_selector_uses_equal_halves_and_resists_midpoint_noise(self):
        for setting in range(27):
            adc = ((2*setting+1)*2048)//28 if setting<14 else 2048+((2*(setting-14)+1)*2048)//26
            self.assertEqual(self.select(adc,255),setting)
            self.assertEqual(self.select(adc,0),setting)
        self.assertEqual(self.select(2047,255),13)
        self.assertEqual(self.select(2048,255),14)
        for adc in (2040,2048,2056,2071):
            self.assertEqual(self.select(adc,13),13)
        self.assertEqual(self.select(2072,13),14)
        for adc in (2024,2040,2048,2056):
            self.assertEqual(self.select(adc,14),14)
        self.assertEqual(self.select(2023,14),13)
        previous, observed = 0,set()
        for adc in range(4096):
            previous = self.select(adc,previous)
            observed.add(previous)
        self.assertEqual(observed,set(range(27)))
        self.assertEqual(self.select(0,26),0)
        self.assertEqual(self.select(4095,0),26)

    def test_physical_top_starts_first_chord_and_downward_steps_follow_order(self):
        for progression,chords in enumerate(PROGRESSIONS):
            low,high = self.generate(14+progression,0)
            self.assertEqual((low[15],high[15]),(chords[0][0],chords[0][0]))
            self.assertEqual(tuple(low[pad] for pad in (15,11,7,3)),
                             tuple(root for root,_ in chords))
            self.assertEqual(tuple(low[pad] for pad in (15,14,13,12)),
                             (chords[0][0],chords[0][0]+(3 if chords[0][1] else 4),
                              chords[0][0]+7,chords[0][0]+12))


@unittest.skipUnless(COMPILER and sys.platform != 'win32','Native C tests use a Unix C compiler')
class Tests(ProgressionScenarios, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        library = Path(cls.directory.name)/'harmony.so'
        subprocess.run([COMPILER,'-shared','-fPIC','-DCELLZ_HOST_TEST','-std=c99',
                        str(ROOT/'firmware/cellz_harmony.c'),'-o',str(library)],check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.harmony_generate.argtypes = [ctypes.c_uint32,ctypes.c_uint32,
                                            ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint16)]
        cls.lib.harmony_bin.argtypes = [ctypes.c_uint32]*3
        cls.lib.harmony_bin.restype = ctypes.c_uint32
        cls.lib.harmony_select.argtypes = [ctypes.c_uint32,ctypes.c_uint32]
        cls.lib.harmony_select.restype = ctypes.c_uint32

    @classmethod
    def tearDownClass(cls):
        cls.lib = None
        cls.directory.cleanup()

    def generate(self,setting,layout):
        low, high = (ctypes.c_uint16*16)(), (ctypes.c_uint16*16)()
        self.lib.harmony_generate(setting,layout,low,high)
        return tuple(low),tuple(high)

    def select(self,adc,previous):
        return self.lib.harmony_select(adc,previous)

    def test_all_maps_have_distinct_pairs_and_two_octave_range(self):
        scales = [(0,2,4,5,7,9,11),(0,2,4,5,7,9,11),
                  (0,2,3,5,7,8,10),(0,2,3,5,7,8,10),
                  (0,2,3,5,7,8,11),(0,2,3,5,7,9,11),
                  (0,2,4,7,9),(0,3,5,7,10),(0,3,5,6,7,10),
                  (0,2,3,5,7,9,10),(0,1,3,5,7,8,10),
                  (0,2,4,6,7,9,11),(0,2,4,5,7,9,10),(0,1,3,5,6,8,10)]
        chords = [(0,4,7),(0,4,7,11),(0,3,7),(0,3,7,10),
                  (0,3,7),(0,3,7),(0,4,7),(0,3,7),(0,3,7),
                  (0,3,7),(0,3,7),(0,4,7),(0,4,7),(0,3,6)]
        for setting in range(14):
            arrangements = set()
            for layout in range(16):
                low, high = (ctypes.c_uint16*16)(), (ctypes.c_uint16*16)()
                self.lib.harmony_generate(setting,layout,low,high)
                self.assertEqual(len(set(zip(low,high))),16)
                self.assertEqual((min(low),max(low)),(0,24))
                self.assertEqual((min(high),max(high)),(0,24))
                self.assertEqual({note%12 for note in high},set(scales[setting]))
                self.assertEqual({note%12 for note in low},set(chords[setting]))
                arrangements.add((tuple(low),tuple(high)))
            self.assertEqual(len(arrangements),16)

    def test_hysteresis_and_full_range(self):
        for count in (14,16):
            previous, observed = 0,set()
            for adc in range(4096):
                previous = self.lib.harmony_bin(adc,count,previous)
                observed.add(previous)
            self.assertEqual(observed,set(range(count)))
            for boundary in range(1,count):
                edge = (boundary*4096+count-1)//count
                for noise in (-8,0,8):
                    self.assertEqual(self.lib.harmony_bin(edge+noise,count,boundary-1),boundary-1)
                    self.assertEqual(self.lib.harmony_bin(edge+noise,count,boundary),boundary)
