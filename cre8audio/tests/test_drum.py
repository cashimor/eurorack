"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk.

Synthetic sequencer and interrupted-save tests; no USB or vendor firmware.
"""
import ctypes as C
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
COMPILER = shutil.which('cc') or shutil.which('gcc')

class Input(C.Structure):
    _fields_ = [('now',C.c_uint32)] + [(n,C.c_uint16) for n in ('touches','tempo','swing')] + [(n,C.c_uint8) for n in ('buttons','alternate','clock_edge')]

class State(C.Structure):
    _fields_ = [('patterns',(C.c_uint16*4)*2)]
    _fields_ += [(n,C.c_uint32) for n in ('last_ms','phase_us','last_edge_ms','external_period_us','pending_at_ms','pulse_until_ms','touch_changed_ms','button_changed_ms')]
    _fields_ += [('pressed_ms',C.c_uint32*2)]
    _fields_ += [(n,C.c_uint32) for n in ('combo_ms','last_step_ms','feedback_until_ms')]
    _fields_ += [(n,C.c_uint16) for n in ('raw_touches','touches','tempo_adc','swing_adc')]
    _fields_ += [(n,C.c_uint8) for n in ('instrument','selected_bank','active_bank','next_step','step','pending_step','external','have_edge','pending','raw_buttons','buttons','combo','combo_saved','dirty','save_requested','saved_status','has_step','outputs','controls_ready')]

class Harness:
    def __init__(self,lib,now=0):
        self.lib,self.state,self.input=lib,State(),Input(now=now)
        lib.drum_init(C.byref(self.state),now)
    def update(self,now,**changes):
        self.input.now=now&0xffffffff
        self.input.clock_edge=0
        for k,v in changes.items(): setattr(self.input,k,v)
        return self.lib.drum_update(C.byref(self.state),C.byref(self.input))
    def edit(self,**changes):
        for k,v in changes.items(): setattr(self.state,k,v)
    def pattern(self,bank,track,value=None):
        if value is not None: self.state.patterns[bank][track]=value
        return self.state.patterns[bank][track]
    def interval(self): return self.lib.drum_interval_us(C.byref(self.state))
    def leds(self,now): return self.lib.drum_led_mask(C.byref(self.state),now)
    def after_save(self,now,success): self.lib.drum_after_save(C.byref(self.state),now,success)
    def click(self,at,button):
        self.update(at,buttons=button); self.update(at+25)
        self.update(at+60,buttons=0); self.update(at+85)

class Scenarios:
    def test_tempo_bounds_and_sixteenth_steps(self):
        for adc,interval in ((0,375000),(4095,62500),(65535,62500)):
            h=self.harness(); h.update(0,tempo=adc)
            self.assertEqual(h.interval(),interval)
            times=[]
            for t in range(1,2001):
                before=h.state.next_step; h.update(t)
                if h.state.next_step!=before: times.append(t)
            self.assertEqual(len(times),2000000//interval)
            self.assertLess(abs(times[-1]*1000-len(times)*interval),1000)
    def test_swing_preserves_pair_duration(self):
        h=self.harness(); h.update(0,tempo=4095,swing=4095)
        self.assertEqual(h.interval(),62500)
        h.update(63); self.assertEqual(h.state.step,0)
        self.assertEqual(h.interval(),93750)
        h.update(157); self.assertEqual(h.state.step,1)
        self.assertEqual(h.interval(),31250)
        h.update(188); self.assertEqual(h.state.step,2)
    def test_button_and_gate_xor_including_button_while_gate_high(self):
        h=self.harness(); h.update(0)
        self.assertEqual(h.state.active_bank,0)
        h.click(10,2); self.assertEqual(h.state.active_bank,1)
        h.update(100,alternate=1); self.assertEqual(h.state.active_bank,0)
        h.click(110,2); self.assertEqual(h.state.active_bank,1)
        h.update(200,alternate=0); self.assertEqual(h.state.active_bank,0)
    def test_editing_toggles_top_pad_once_and_targets_audible_bank(self):
        h=self.harness(); original=h.pattern(0,0)
        h.update(0,touches=0x8000); h.update(11)
        self.assertEqual(h.pattern(0,0),original)
        h.update(12); self.assertEqual(h.pattern(0,0),original^1)
        h.click(20,1); self.assertEqual(h.state.instrument,1)
        self.assertEqual(h.pattern(0,1),0x1010) # held pad does not retoggle
        h.update(110,touches=0); h.update(122)
        h.update(123,alternate=1,touches=1); h.update(135)
        self.assertEqual(h.pattern(1,1),0xf010^0x8000)
        self.assertEqual(h.state.dirty,1)
    def test_all_tracks_play_independently_of_editor(self):
        h=self.harness()
        for track in range(4): h.pattern(0,track,1<<track)
        h.edit(instrument=3)
        for step in range(4):
            self.assertEqual(h.update(100+step*100,clock_edge=1),1<<step)
            self.assertEqual(h.update(105+step*100),0)
    def test_external_clock_override_debounce_and_timeout(self):
        h=self.harness(); h.update(100,clock_edge=1)
        self.assertEqual(h.state.next_step,1)
        h.update(110,clock_edge=1); self.assertEqual(h.state.next_step,1)
        h.update(200,clock_edge=1); self.assertEqual(h.state.next_step,2)
        h.update(700); self.assertEqual(h.state.external,1)
        h.update(701); self.assertEqual(h.state.external,0)
        h.update(1075); self.assertEqual(h.state.next_step,2)
        h.update(1076); self.assertEqual(h.state.next_step,3)
    def test_external_swing_delays_odd_steps_without_extra_steps(self):
        h=self.harness(); h.update(100,swing=4095,clock_edge=1)
        h.update(200,clock_edge=1)
        self.assertEqual((h.state.step,h.state.pending),(0,1))
        h.update(249); self.assertEqual(h.state.step,0)
        h.update(250); self.assertEqual((h.state.step,h.state.pending),(1,0))
        h.update(300,clock_edge=1); self.assertEqual(h.state.step,2)
    def test_stalls_do_not_replay_a_burst(self):
        h=self.harness(); h.update(0,tempo=4095)
        h.update(2000); self.assertEqual(h.state.next_step,1)
        h.update(2000); self.assertEqual(h.state.next_step,1)
        h.update(2001); self.assertEqual(h.state.next_step,1)
    def test_clock_and_pulse_expiry_across_millisecond_wrap(self):
        h=self.harness(0xfffffff0)
        h.update(0xfffffffe,clock_edge=1)
        self.assertNotEqual(h.state.outputs,0)
        h.update(2); self.assertNotEqual(h.state.outputs,0)
        h.update(3); self.assertEqual(h.state.outputs,0)
        h.update(98,clock_edge=1); self.assertEqual(h.state.next_step,2)
    def test_save_is_explicit_once_and_does_not_click_buttons(self):
        h=self.harness(); h.update(0,buttons=3); h.update(20)
        h.update(1519); self.assertEqual(h.state.save_requested,0)
        h.update(1520); self.assertEqual(h.state.save_requested,1)
        h.edit(dirty=1); h.after_save(1520,0)
        self.assertEqual((h.state.saved_status,h.state.dirty),(2,1))
        h.update(2000); self.assertEqual(h.state.save_requested,0)
        h.update(2001,buttons=0); h.update(2021)
        self.assertEqual((h.state.selected_bank,h.state.instrument),(0,0))
        h.after_save(2021,1); self.assertEqual((h.state.saved_status,h.state.dirty),(1,0))
    def test_leds_show_selected_pattern_and_never_light_disabled_steps(self):
        h=self.harness(); h.pattern(0,0,1)
        self.assertEqual(h.leds(0),0x8000)
        h.update(100,clock_edge=1); self.assertEqual(h.leds(100),0)
        self.assertEqual(h.leds(120),0x8000)
        h.update(200,clock_edge=1); self.assertEqual(h.leds(200),0x8000)

ERASE=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32)
PROGRAM=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32)
class IO(C.Structure): _fields_=[('context',C.c_void_p),('erase',ERASE),('program',PROGRAM)]
class Journal:
    def __init__(self,lib):
        self.lib=lib; self.pages=[(C.c_uint8*1024)(*[255]*1024) for _ in range(2)]
        self.writes=0; self.erases=[]; self.fail_half=None; self.erase_failure=False
        self.erase_callback=ERASE(self.erase); self.program_callback=PROGRAM(self.program)
        self.io=IO(None,self.erase_callback,self.program_callback)
    def erase(self,context,page):
        self.erases.append(page)
        if self.erase_failure: return 0
        for i in range(1024): self.pages[page][i]=255
        return 1
    def program(self,context,page,offset,value):
        for half in range(2):
            if self.fail_half==self.writes: return 0
            for i in range(2):
                at=offset+half*2+i; byte=(value>>(8*(half*2+i)))&255
                self.pages[page][at]&=byte
            self.writes+=1
        return 1
    def save(self,values):
        return self.lib.drum_store_save(*self.pages,(C.c_uint16*8)(*values),C.byref(self.io))
    def load(self):
        values=(C.c_uint16*8)(); sequence=C.c_uint32()
        ok=self.lib.drum_store_load(*self.pages,values,C.byref(sequence))
        return (list(values),sequence.value) if ok else None

class StoreScenarios:
    def test_erased_boot_has_no_record_and_explicit_save_loads_all_patterns(self):
        j=Journal(self.lib); self.assertIsNone(j.load()); self.assertEqual(j.writes,0)
        self.assertEqual(j.save(range(8)),1); self.assertEqual(j.load(),(list(range(8)),1))
        self.assertEqual(j.erases,[])
    def test_power_loss_after_every_halfword_preserves_previous_committed_record(self):
        for cut in range(18):
            j=Journal(self.lib); j.save(range(8)); j.fail_half=j.writes+cut
            self.assertEqual(j.save(range(8,16)),0)
            self.assertEqual(j.load(),(list(range(8)),1))
            j.fail_half=None; self.assertEqual(j.save(range(16,24)),1)
            self.assertEqual(j.load(),(list(range(16,24)),2))
    def test_rollover_erases_only_other_page_and_survives_every_interrupted_word(self):
        for cut in range(18):
            j=Journal(self.lib)
            for n in range(28): self.assertEqual(j.save([n]*8),1)
            j.fail_half=j.writes+cut; self.assertEqual(j.save([28]*8),0)
            self.assertEqual(j.erases,[1]); self.assertEqual(j.load(),([27]*8,28))
    def test_erase_failure_does_not_destroy_latest_record(self):
        j=Journal(self.lib)
        for n in range(28): j.save([n]*8)
        j.erase_failure=True; self.assertEqual(j.save([28]*8),0)
        self.assertEqual(j.load(),([27]*8,28))
    def test_crc_corruption_falls_back_and_unknown_data_is_not_erased(self):
        j=Journal(self.lib); j.save(range(8)); j.save(range(8,16))
        j.pages[0][36+8]^=1; self.assertEqual(j.load(),(list(range(8)),1))
        j=Journal(self.lib)
        j.pages[0][0]=0; j.pages[1][0]=0
        self.assertEqual(j.save(range(8)),0); self.assertEqual((j.writes,j.erases),(0,[]))
    def test_interrupted_first_save_can_recover_on_other_blank_page(self):
        j=Journal(self.lib); j.fail_half=1
        self.assertEqual(j.save(range(8)),0); self.assertIsNone(j.load())
        j.fail_half=None; self.assertEqual(j.save(range(8)),1)
        self.assertEqual(j.load(),(list(range(8)),1))
    def test_multiple_rollovers_and_sequence_wrap(self):
        import struct,zlib
        j=Journal(self.lib)
        for n in range(90): self.assertEqual(j.save([n]*8),1)
        self.assertEqual(j.load(),([89]*8,90)); self.assertEqual(j.erases,[1,0,1])
        # Manufacture a complete valid last record at sequence UINT32_MAX.
        j=Journal(self.lib); j.save(range(8))
        raw=bytearray(bytes(j.pages[0][:36])); struct.pack_into('<I',raw,4,0xffffffff)
        struct.pack_into('<I',raw,24,zlib.crc32(raw[:24]))
        j.pages[0][:36]=raw
        self.assertEqual(j.save([42]*8),1); self.assertEqual(j.load(),([42]*8,0))

@unittest.skipUnless(COMPILER and sys.platform!='win32','Native C tests use a Unix C compiler')
class NativeTests(Scenarios,StoreScenarios,unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory=tempfile.TemporaryDirectory(); path=Path(cls.directory.name)/'drum.so'
        subprocess.run([COMPILER,'-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror',str(ROOT/'firmware/drum_engine.c'),str(ROOT/'firmware/drum_store.c'),'-o',str(path)],check=True)
        cls.lib=C.CDLL(str(path))
        cls.lib.drum_init.argtypes=[C.POINTER(State),C.c_uint32]
        cls.lib.drum_update.argtypes=[C.POINTER(State),C.POINTER(Input)]
        cls.lib.drum_interval_us.argtypes=[C.POINTER(State)]
        cls.lib.drum_led_mask.argtypes=[C.POINTER(State),C.c_uint32]
        cls.lib.drum_after_save.argtypes=[C.POINTER(State),C.c_uint32,C.c_uint32]
    @classmethod
    def tearDownClass(cls):
        cls.lib=None; cls.directory.cleanup()
    def harness(self,now=0): return Harness(self.lib,now)

if __name__=='__main__': unittest.main()
