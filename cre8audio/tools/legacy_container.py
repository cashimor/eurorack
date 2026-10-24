# Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of Cre8audio gear or other equipment resulting from use. Use at your own risk.
from pathlib import Path

def legacy_parse(path):
    # btn_upFile_Click: Join(ReadAllLines), Split(';'), then after[4].Split('=').
    fields = ''.join(path.read_text(encoding='ascii').splitlines()).split(';')
    version = fields[0].split(':')[2].split('V')[1]
    vid, dfu_pid, app_pid = (int(fields[i].split(':')[1], 16) for i in (1, 2, 3))
    images = fields[4].split('=')
    # StrToHexByte permits whitespace, but these supplied bodies have none.
    return version, vid, dfu_pid, app_pid, [bytes.fromhex(s) for s in images]
