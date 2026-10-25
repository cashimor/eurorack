# Independent Cre8audio Cellz tools and firmware

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

This directory contains an independently written Windows updater and the source
for **Cellz Harmony prototype 5**. The updater provides a Windows route through
the old bootloader migration, followed by normal HID application updates, without
requiring a Macintosh. This is an experimental, version-specific workflow.

Harmony turns the two pots into a shared harmony selector and a note-layout
selector. Both voices get deterministic notes at startup; the upper voice uses a
full scale and the lower voice uses chord tones. The first half of the harmony
control selects 14 scale settings; the other half selects 13 four-chord
progressions. There are 16 variations per setting, with no duplicate complete
note pair within a map. Progression variations change melodic choices from each
chord's full scale as well as pad placement. The initial
P1 firmware was installed on one modified spare Cellz and reported to work excellently.
P3 includes independent Tune-button tap clocks, three-tap startup, hold-to-stop,
and simple tempo ratios through 1:8. Accepted tempo settings align both running
clocks to a shared cycle without adding an immediate tap-triggered step. P2 and
P3 have been installed with startup signatures verified; P3's musical behavior
still awaits user verification.
P4 adds the progression bank and has passed offline ARM tests for all 432 maps.
It has been installed on the same spare with all 26 staging blocks verified,
checksum accepted and a fresh HARMONY4 signature confirmed. Musical verification
of the progression bank and harmony-change reset is still pending.
P5 corrects the physical orientation: hardware pad 15 is the top, so resets and
variation-zero progressions start there and follow the downwards clock paths.
P5 has also been installed with staging, checksum and startup verification;
physical confirmation of the corrected top position is pending.

**Drum D2** adds Shift-edited accents on the two CV outputs and three-hit triplets
spanning adjacent step pairs. Right-button Shift edits accents; left-button Shift
edits triplets. Shift gestures keep steps enabled and consume the button click.
Turning a step off clears its accent and breaks its triplet. Two flash journal
pages save both banks, accents and triplets, with backward loading of D1 patterns.
D1 was installed and verified on the same spare; D2 is a separate offline candidate
with hardware verification pending. See the [Drum manual](docs/drum.md).

## Start here

- [Windows updater and bootloader migration](docs/windows-updater.md)
- [Harmony controls, building and installation](docs/harmony.md)
- [Separate Drum sequencer: controls, pattern saving and building](docs/drum.md)
- [Validation and supported scope](docs/validation.md)
- [Responsibility and redistribution notice](NOTICE.md)

All `flash` and `migrate` commands are **offline plans unless `--execute` is
present**. `inspect` performs identification reads. Updates validate source files,
require a unique device, verify RAM before every HID flash commit, and check the
whole application checksum before launching it. They do not install USB drivers
or retry uncertain writes automatically.

## What is included

- `tools/`: our Python updater, protocol implementation, container handling and
  separate Harmony and Drum builders. Python 3.11+; live USB support targets Windows.
- `firmware/`: our C extension and linker script, integrated into a separately
  supplied official V0.0.3.0 application at build time.
- `tests/`: synthetic host/protocol tests that need no hardware or vendor files.

**No Cre8audio firmware, updater binaries, extracted code, disassembly, manuals,
logos, or other vendor assets are included.** Obtain the official package yourself
from [Cre8audio's firmware page](https://www.cre8audio.com/firmware-and-software).
Generated DFU files contain vendor application code and must remain local;
they are deliberately excluded from this repository.

Our source and documentation in this directory are under the [MIT license](LICENSE).
That license does not grant rights to Cre8audio's files or alter the licensing of
other directories in this repository.

## Run the source-only tests

From this directory:

```powershell
python -m unittest discover -s tests -v
```

The primary tested hardware is Cellz. Chipz and NiftyCASE are not supported by this
public command-line tool yet. Do not select them in a driver tool or substitute
their firmware in the Cellz instructions.
