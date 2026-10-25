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

class Triplet(C.Structure):
    _fields_=[('started_ms',C.c_uint32),('period_us',C.c_uint32)]+[(n,C.c_uint8) for n in ('active','next_hit','step','bank','accent')]

class State(C.Structure):
    _fields_ = [(n,(C.c_uint16*4)*2) for n in ('patterns','accents','triplets')]
    _fields_ += [(n,C.c_uint32) for n in ('last_ms','phase_us','last_edge_ms','external_period_us','pending_at_ms')]
    _fields_ += [('pulse_until_ms',C.c_uint32*4)]+[(n,C.c_uint32) for n in ('touch_changed_ms','button_changed_ms')]
    _fields_ += [('pressed_ms',C.c_uint32*2)]
    _fields_ += [(n,C.c_uint32) for n in ('combo_ms','last_step_ms','feedback_until_ms')]
    _fields_ += [('group',Triplet*4)]
    _fields_ += [(n,C.c_uint16) for n in ('raw_touches','touches','tempo_adc','swing_adc')]
    _fields_ += [(n,C.c_uint8) for n in ('instrument','selected_bank','active_bank','next_step','step','pending_step','external','have_edge','pending','raw_buttons','buttons','combo','combo_saved','dirty','save_requested','saved_status','has_step','outputs','controls_ready','accent_outputs','shift_used','combo_edited')]

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
    def mask(self,kind,bank,track,value=None):
        field=getattr(self.state,kind)
        if value is not None: field[bank][track]=value
        return field[bank][track]
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
        self.assertEqual(h.pattern(0,1),0x4040) # held pad does not retoggle
        h.update(110,touches=0); h.update(122)
        h.update(123,alternate=1,touches=1); h.update(135)
        self.assertEqual(h.pattern(1,1),0xc444^0x8000)
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
        self.assertEqual(h.leds(160),0x8000)
        h.update(200,clock_edge=1); self.assertEqual(h.leds(200),0x8000)

class EditScenarios:
    def touch(self,h,at,step,buttons=0):
        h.update(at,buttons=buttons,touches=1<<(15-step));h.update(at+12)
        h.update(at+13,touches=0);h.update(at+25)
    def shift_touch(self,h,at,step,button):
        h.update(at,buttons=button)
        self.touch(h,at+25,step,button)
        h.update(at+60,buttons=0);h.update(at+85)
    def test_right_shift_enables_and_toggles_accent_without_switching_bank(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,0)
        self.shift_touch(h,0,0,2)
        self.assertEqual((h.pattern(0,2),h.mask('accents',0,2)),(1,1))
        self.assertEqual(h.state.selected_bank,0)
        self.shift_touch(h,100,0,2)
        self.assertEqual((h.pattern(0,2),h.mask('accents',0,2)),(1,0))
    def test_simultaneous_shift_and_touch_cannot_unselect_or_click_on_release(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,1)
        h.update(0,buttons=2,touches=0x8000);h.update(12)
        self.assertEqual(h.pattern(0,2),1)
        h.update(20);h.update(30,buttons=0,touches=0);h.update(55)
        self.assertEqual((h.state.selected_bank,h.mask('accents',0,2)),(0,1))
    def test_accent_shift_on_gate_tracks_never_unselects_or_accents(self):
        h=self.harness();h.pattern(0,0,1)
        self.shift_touch(h,0,0,2)
        self.assertEqual((h.pattern(0,0),h.mask('accents',0,0)),(1,0))
        self.assertEqual(h.state.selected_bank,0)
    def test_left_shift_enables_pair_and_does_not_change_instrument(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,0)
        self.shift_touch(h,0,4,1)
        self.assertEqual((h.pattern(0,2),h.mask('triplets',0,2)),(0x30,0x10))
        self.assertEqual(h.state.instrument,2)
        self.shift_touch(h,100,4,1)
        self.assertEqual((h.pattern(0,2),h.mask('triplets',0,2)),(0x30,0))
    def test_triplet_wraps_step_sixteen_to_step_one(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,0)
        self.shift_touch(h,0,15,1)
        self.assertEqual((h.pattern(0,2),h.mask('triplets',0,2)),(0x8001,0x8000))
    def test_overlapping_triplet_moves_group_and_preserves_other_steps(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,0)
        self.shift_touch(h,0,3,1);self.shift_touch(h,100,4,1)
        self.assertEqual((h.pattern(0,2),h.mask('triplets',0,2)),(0x38,0x10))
    def test_accent_on_either_triplet_pad_accents_whole_group(self):
        h=self.harness();h.edit(instrument=3);h.pattern(0,3,0)
        self.shift_touch(h,0,2,1);self.shift_touch(h,100,3,2)
        self.assertEqual(h.mask('accents',0,3),0x0c)
        self.shift_touch(h,200,2,2)
        self.assertEqual((h.pattern(0,3),h.mask('accents',0,3)),(0x0c,0))
    def test_pairing_an_accented_single_propagates_accent(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,0)
        self.shift_touch(h,0,1,2);self.shift_touch(h,100,0,1)
        self.assertEqual(h.mask('accents',0,2),3)
    def test_unselect_either_member_removes_its_accent_and_breaks_triplet(self):
        for step in (0,1):
            h=self.harness();h.edit(instrument=2);h.pattern(0,2,3)
            h.mask('accents',0,2,3);h.mask('triplets',0,2,1)
            self.touch(h,0,step)
            self.assertEqual(h.pattern(0,2),3^(1<<step))
            self.assertEqual(h.mask('accents',0,2),3^(1<<step))
            self.assertEqual(h.mask('triplets',0,2),0)
    def test_shift_views_show_accents_or_triplet_members_and_release_returns_steps(self):
        h=self.harness();h.edit(instrument=2);h.pattern(0,2,7)
        h.mask('accents',0,2,1);h.mask('triplets',0,2,1)
        h.update(0,buttons=2);self.assertEqual(h.leds(0),0x8000)
        h.update(25,buttons=0);h.update(50);self.assertEqual(h.leds(50),0xe000)
        h.update(60,buttons=1);self.assertEqual(h.leds(60),0xc000)
    def test_pad_with_both_buttons_cannot_edit_or_save_until_released(self):
        h=self.harness();before=h.pattern(0,0)
        h.update(0,buttons=3,touches=0x8000);h.update(20)
        h.update(40,touches=0);h.update(60);h.update(1600)
        self.assertEqual((h.pattern(0,0),h.state.save_requested),(before,0))
        h.update(1700,buttons=0);h.update(1725)
        self.assertEqual((h.state.instrument,h.state.selected_bank),(0,0))
    def test_triplet_three_hits_over_two_steps_and_other_track_stays_straight(self):
        h=self.harness();h.update(0,tempo=4095)
        for track in range(4):h.pattern(0,track,0)
        h.pattern(0,2,3);h.mask('triplets',0,2,1);h.pattern(0,0,3)
        hits=[[],[]]
        for now in range(1,189):
            before=h.state.outputs;h.update(now)
            for index,track in enumerate((0,2)):
                if h.state.outputs&(1<<track) and not before&(1<<track):hits[index].append(now)
        self.assertEqual(hits[0],[63,125])
        self.assertEqual(hits[1],[63,105,147])
    def test_accented_triplet_all_three_hits_have_accent_level(self):
        h=self.harness();h.update(0,tempo=4095);h.pattern(0,3,3)
        h.mask('triplets',0,3,1);h.mask('accents',0,3,3)
        h.update(63);self.assertTrue(h.state.accent_outputs&8)
        h.update(68);self.assertFalse(h.state.outputs&8)
        h.update(105);self.assertTrue(h.state.accent_outputs&8)
        h.update(110);h.update(125);h.update(147);self.assertTrue(h.state.accent_outputs&8)
    def test_external_triplet_uses_clock_period_and_suppresses_partner(self):
        h=self.harness();h.pattern(0,2,0x0c);h.mask('triplets',0,2,4)
        for track in (0,1,3):h.pattern(0,track,0)
        h.update(0,clock_edge=1);h.update(100,clock_edge=1)
        h.update(200,clock_edge=1);self.assertTrue(h.state.outputs&4)
        h.update(205);h.update(266);self.assertFalse(h.state.outputs&4)
        h.update(267);self.assertTrue(h.state.outputs&4)
        h.update(272);h.update(300,clock_edge=1);self.assertFalse(h.state.outputs&4)
        h.update(334);self.assertTrue(h.state.outputs&4)
    def test_stalled_triplet_skips_overdue_hit_without_burst(self):
        h=self.harness();h.update(0);h.pattern(0,2,3);h.mask('triplets',0,2,1)
        h.update(375);h.update(380)
        h.update(900);self.assertTrue(h.state.outputs&4)
        h.update(905);self.assertFalse(h.state.outputs&4)
        h.update(905);self.assertFalse(h.state.outputs&4)
        self.assertEqual(h.state.group[2].next_hit,3)
    def test_per_track_pulses_do_not_truncate_another_output(self):
        h=self.harness();h.update(0,tempo=4095)
        for track in range(4):h.pattern(0,track,0)
        h.pattern(0,2,3);h.mask('triplets',0,2,1);h.pattern(0,0,2)
        h.update(63);h.update(68);h.update(105)
        self.assertTrue(h.state.outputs&4)
        h.edit(phase_us=60500);h.update(107)
        self.assertEqual(h.state.outputs&5,5)
        h.update(110);self.assertEqual(h.state.outputs&5,1)
        h.update(112);self.assertEqual(h.state.outputs&5,0)
    def test_bank_switch_cancels_pending_triplet_hits(self):
        h=self.harness();h.update(0);h.pattern(0,2,3);h.mask('triplets',0,2,1)
        h.update(375);h.update(380);h.update(400,alternate=1)
        self.assertEqual(h.state.group[2].active,0)
        h.update(625);self.assertFalse(h.state.outputs&4)

    def test_preheld_pad_also_cancels_both_button_save(self):
        h=self.harness();h.update(0,touches=0x8000);h.update(12)
        h.update(20,buttons=3);h.update(40)
        h.update(50,touches=0);h.update(70);h.update(1540)
        self.assertEqual(h.state.save_requested,0)

    def test_triplets_ignore_swing_within_their_two_step_span(self):
        for start,expected in ((0,[63,105,147]),(1,[157,199,241])):
            h=self.harness();h.update(0,tempo=4095,swing=4095)
            for track in range(4):h.pattern(0,track,0)
            h.pattern(0,2,(1<<start)|(1<<(start+1)));h.mask('triplets',0,2,1<<start)
            hits=[]
            for now in range(1,283):
                before=h.state.outputs;h.update(now)
                if h.state.outputs&4 and not before&4:hits.append(now)
            self.assertEqual(hits,expected)
    def test_wrapping_triplet_stays_three_hits_across_bar_boundary(self):
        h=self.harness();h.update(0,tempo=4095)
        for track in range(4):h.pattern(0,track,0)
        h.pattern(0,2,0x8001);h.mask('triplets',0,2,0x8000)
        hits=[]
        for now in range(1,1126):
            before=h.state.outputs;h.update(now)
            if h.state.outputs&4 and not before&4:hits.append(now)
        self.assertEqual(hits,[63,1000,1042,1084])
    def test_simultaneous_triplets_on_all_tracks_keep_independent_accents(self):
        h=self.harness();h.update(0,tempo=4095)
        for track in range(4):
            h.pattern(0,track,3);h.mask('triplets',0,track,1)
        h.mask('accents',0,3,3)
        for now in (63,105,147):
            h.update(now);self.assertEqual(h.state.outputs,15)
            self.assertEqual(h.state.accent_outputs,8)
            h.update(now+5);self.assertEqual(h.state.outputs,0)
    def test_triplet_timing_survives_millisecond_wrap(self):
        start=0xffffffc0;h=self.harness(start);h.update(start,tempo=4095)
        for track in range(4):h.pattern(0,track,0)
        h.pattern(0,2,3);h.mask('triplets',0,2,1)
        h.update(start+63);h.update(start+68)
        h.update(start+105);self.assertTrue(h.state.outputs&4)
        h.update(start+110);h.update(start+125)
        h.update(start+147);self.assertTrue(h.state.outputs&4)

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
        return self.lib.drum_store_save(*self.pages,(C.c_uint16*24)(*values),C.byref(self.io))
    def load(self):
        values=(C.c_uint16*24)(); sequence=C.c_uint32()
        ok=self.lib.drum_store_load(*self.pages,values,C.byref(sequence))
        return (list(values),sequence.value) if ok else None

class StoreScenarios:
    def test_erased_boot_has_no_record_and_explicit_save_loads_all_patterns(self):
        j=Journal(self.lib); self.assertIsNone(j.load()); self.assertEqual(j.writes,0)
        self.assertEqual(j.save(range(24)),1); self.assertEqual(j.load(),(list(range(24)),1))
        self.assertEqual(j.erases,[])
    def test_power_loss_after_every_halfword_preserves_previous_committed_record(self):
        for cut in range(34):
            j=Journal(self.lib); j.save(range(24)); j.fail_half=j.writes+cut
            self.assertEqual(j.save(range(24,48)),0)
            self.assertEqual(j.load(),(list(range(24)),1))
            j.fail_half=None; self.assertEqual(j.save(range(48,72)),1)
            self.assertEqual(j.load(),(list(range(48,72)),2))
    def test_rollover_erases_only_other_page_and_survives_every_interrupted_word(self):
        for cut in range(34):
            j=Journal(self.lib)
            for n in range(15): self.assertEqual(j.save([n]*24),1)
            j.fail_half=j.writes+cut; self.assertEqual(j.save([15]*24),0)
            self.assertEqual(j.erases,[1]); self.assertEqual(j.load(),([14]*24,15))
    def test_erase_failure_does_not_destroy_latest_record(self):
        j=Journal(self.lib)
        for n in range(15): j.save([n]*24)
        j.erase_failure=True; self.assertEqual(j.save([15]*24),0)
        self.assertEqual(j.load(),([14]*24,15))
    def test_crc_corruption_falls_back_and_unknown_data_is_not_erased(self):
        j=Journal(self.lib); j.save(range(24)); j.save(range(24,48))
        j.pages[0][68+8]^=1; self.assertEqual(j.load(),(list(range(24)),1))
        j=Journal(self.lib)
        j.pages[0][0]=0; j.pages[1][0]=0
        self.assertEqual(j.save(range(24)),0); self.assertEqual((j.writes,j.erases),(0,[]))
    def test_interrupted_first_save_can_recover_on_other_blank_page(self):
        j=Journal(self.lib); j.fail_half=1
        self.assertEqual(j.save(range(24)),0); self.assertIsNone(j.load())
        j.fail_half=None; self.assertEqual(j.save(range(24)),1)
        self.assertEqual(j.load(),(list(range(24)),1))
    def test_multiple_rollovers_and_sequence_wrap(self):
        import struct,zlib
        j=Journal(self.lib)
        for n in range(90): self.assertEqual(j.save([n]*24),1)
        self.assertEqual(j.load(),([89]*24,90)); self.assertEqual(j.erases,[1,0,1,0,1])
        # Manufacture a complete valid last record at sequence UINT32_MAX.
        j=Journal(self.lib); j.save(range(24))
        raw=bytearray(bytes(j.pages[0][:68])); struct.pack_into('<I',raw,4,0xffffffff)
        struct.pack_into('<I',raw,56,zlib.crc32(raw[:56]))
        j.pages[0][:68]=raw
        self.assertEqual(j.save([42]*24),1); self.assertEqual(j.load(),([42]*24,0))

    def legacy_record(self,j,page=0,slot=0,sequence=7):
        import struct,zlib
        raw=bytearray(36);struct.pack_into('<II8H',raw,0,0x314d5244,sequence,*range(8))
        struct.pack_into('<III',raw,24,zlib.crc32(raw[:24]),1,0x454e4f44)
        j.pages[page][slot*36:slot*36+36]=raw
    def test_d1_load_initializes_accent_triplet_masks_without_writing(self):
        j=Journal(self.lib);self.legacy_record(j,slot=3)
        self.assertEqual(j.load(),(list(range(8))+[0]*16,7))
        self.assertEqual((j.writes,j.erases),(0,[]))
    def test_first_d2_save_migrates_to_other_page_and_retains_d1(self):
        j=Journal(self.lib);self.legacy_record(j,slot=3)
        original=bytes(j.pages[0]);self.assertEqual(j.save(range(24)),1)
        self.assertEqual(j.erases,[1]);self.assertEqual(bytes(j.pages[0]),original)
        self.assertEqual(j.load(),(list(range(24)),8))
    def test_every_interrupted_migration_retains_d1_patterns(self):
        for cut in range(34):
            j=Journal(self.lib);self.legacy_record(j,slot=27)
            old=bytes(j.pages[0]);j.fail_half=cut
            self.assertEqual(j.save(range(24)),0)
            self.assertEqual(j.load(),(list(range(8))+[0]*16,7))
            self.assertEqual(bytes(j.pages[0]),old)
    def test_interrupted_migration_erase_preserves_d1_latest(self):
        j=Journal(self.lib);self.legacy_record(j,page=1,sequence=99)
        j.erase_failure=True;self.assertEqual(j.save(range(24)),0)
        self.assertEqual(j.load(),(list(range(8))+[0]*16,99))
    def test_mixed_pages_select_newest_sequence_and_ignore_incomplete_d2(self):
        j=Journal(self.lib);self.legacy_record(j,page=1,sequence=3)
        self.assertEqual(j.save(range(24)),1)
        self.assertEqual(j.load(),(list(range(24)),4))
        j.pages[0][64]=0 # invalidate D2 commit, fall back to D1
        self.assertEqual(j.load(),(list(range(8))+[0]*16,3))

@unittest.skipUnless(COMPILER and sys.platform!='win32','Native C tests use a Unix C compiler')
class NativeTests(Scenarios,EditScenarios,StoreScenarios,unittest.TestCase):
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
