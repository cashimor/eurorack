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
