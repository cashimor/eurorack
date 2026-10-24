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


@unittest.skipUnless(COMPILER and sys.platform != 'win32','Native C tests use a Unix C compiler')
class Tests(unittest.TestCase):
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

    @classmethod
    def tearDownClass(cls):
        cls.lib = None
        cls.directory.cleanup()

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
