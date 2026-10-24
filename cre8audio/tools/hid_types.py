# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use of this project. Use it at your own risk.
import ctypes as c
from ctypes import wintypes as w

class GUID(c.Structure):
    _fields_ = [('a', w.DWORD), ('b', w.WORD), ('d', w.WORD), ('e', c.c_ubyte * 8)]

class Interface(c.Structure):
    _fields_ = [('size', w.DWORD), ('guid', GUID), ('flags', w.DWORD), ('reserved', c.c_void_p)]

class Attributes(c.Structure):
    _fields_ = [('size', w.DWORD), ('vid', w.WORD), ('pid', w.WORD), ('version', w.WORD)]

class Caps(c.Structure):
    _fields_ = [('usage', w.WORD), ('page', w.WORD), ('input', w.WORD), ('output', w.WORD),
                ('feature', w.WORD), ('reserved', w.WORD * 17), ('counts', w.WORD * 10)]
