"""Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction
of Cre8audio gear or other equipment resulting from use. Use at your own risk."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'tools'))
from hid_protocol import drum_status
class DrumStatusTests(unittest.TestCase):
    def report(self):
        r=bytearray(64); r[16:24]=b'DRUM0001'; r[24:31]=bytes([3,0,1,15,1,1,2]); return r
    def test_decodes_editor_playback_and_failed_unsaved_save(self):
        self.assertEqual(drum_status(self.report()),{'instrument':4,'selected_bank':'A','playing_and_editing_bank':'B','step':16,'external_clock':True,'unsaved_edits':True,'last_save':'failed'})
    def test_no_step_and_success(self):
        r=self.report(); r[27]=255; r[29]=0; r[30]=1
        s=drum_status(r); self.assertIsNone(s['step']); self.assertFalse(s['unsaved_edits']); self.assertEqual(s['last_save'],'success')
    def test_other_applications_and_truncated_reports_are_not_drum(self):
        r=self.report(); r[16:24]=b'HARMONY5'
        self.assertIsNone(drum_status(r)); self.assertIsNone(drum_status(self.report()[:40]))
    def test_invalid_drum_status_is_rejected(self):
        r=self.report(); r[24]=4
        with self.assertRaises(ValueError): drum_status(r)

    def test_d2_uses_the_same_readonly_status_layout(self):
        r=self.report();r[16:24]=b'DRUM0002'
        self.assertEqual(drum_status(r)['instrument'],4)

    def test_d3_reports_length_page_gate_inversion_and_swing_pickup(self):
        r=self.report();r[16:24]=b'DRUM0003';r[27]=31;r[31:34]=bytes([32,1,1])
        s=drum_status(r)
        self.assertEqual((s['length_steps'],s['step']),(32,32))
        self.assertEqual((s['selected_page'],s['playing_and_editing_page']),('A2','B2'))
        self.assertTrue(s['swing_pickup'])

    def test_d3_rejects_inconsistent_length_page_and_step(self):
        for length,page,step,pickup in ((24,0,0,0),(16,1,0,0),(32,2,0,0),(16,0,16,0),(32,0,32,0),(32,0,0,2)):
            r=self.report();r[16:24]=b'DRUM0003';r[27]=step;r[31:34]=bytes([length,page,pickup])
            with self.assertRaises(ValueError): drum_status(r)

    def test_older_signatures_still_reject_step_32(self):
        for signature in (b'DRUM0001',b'DRUM0002'):
            r=self.report();r[16:24]=signature;r[27]=31
            with self.assertRaises(ValueError): drum_status(r)
