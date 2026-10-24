# Validation and limits

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

## Development hardware

One modified spare Cellz was migrated from old application revision `001b` through
legacy loader `0003` to HID loader `0102`. The initial software handoff returned to
the migration application. The switch beside USB subsequently entered the new
loader. The official V0.0.3.0 image was installed, its whole-image checksum `3a9a`
accepted, and a fresh application version report obtained.

Harmony P1 was then installed through software-requested loader entry, without
another migration or USB disconnection. All 24 RAM staging blocks were read back
before commits, the loader accepted checksum `f6dd`, and the restarted application
returned `HARMONY1`. The owner reported that it worked excellently. This is one
hardware trial, not a compatibility claim for every Cellz revision or wiring setup.

The original source-only public builder reproduced the exact installed P1 image hash. The
packaged tool refused to proceed when no unique device was connected during the
initial publication check. The packaged updater subsequently installed P2 and P3
on the same spare: all 25 RAM staging blocks were read back for each update,
checksums `f790` and `e204` were accepted, and fresh `HARMONY2` and `HARMONY3`
application reports were confirmed. P2's live musical trial prompted the shared
cycle-boundary change. P3's musical behavior still awaits user verification.
Do not treat successful enumeration as proof of recoverability or flash contents.

Harmony P3 has also passed offline tests. Its
compiled ARM tests exercise three-tap startup, averaging, irregular taps, debounce,
hold/release boundaries, independent and simultaneous buttons, 1:8 and inverse
ratios, 120/35 BPM snapping, counter wraparound, fractional periods and stalled
loop handling. Integration checks preserve the P1 musical/ADC/external-clock
behavior, verify the patched SysTick vector and original countdowns, check new RAM
boundaries, and run the whole P3 image through the real loader in emulation.
Regression cases check shared cycles regardless of button order, retapping just
after a step, continued taps exactly on clock edges, and stopping on a due edge.
Software-pended EXTI dispatch is modeled; actual interrupt timing and physical
left/right button mapping still need further hardware verification.

P4 was installed on the same spare using the packaged updater. All 26 staging
blocks were read back, checksum `2784` accepted, and a fresh `HARMONY4` application
report confirmed. Its progression behavior and harmony-change reset still await
user verification. Its compiled ARM suite checks all 432 maps
(14 scales and 13 progressions, with 16 variations each), distinct note pairs,
per-chord major/minor membership, all seven melody degrees across variations,
root/third/fifth/octave chord groups, and equal half-range control banks with midpoint
hysteresis. All 27 settings and 16 variations are applied through the real loop
hook. Original playback and both external-clock paths are checked for every
progression, and the whole P4 image passes the original loader's checksum and
flash-mapping logic in emulation. The P3 clock synchronization regressions also
pass with the P4 image.
Harmony-change regressions also check every starting pad against every setting,
the first-pad pitch outputs and indicator, retained tempos with reset phases,
absence of an immediate internal step, and variation-only position preservation.

P4's live trial revealed that hardware pad 0 is at the physical bottom. P5 uses
pad 15 for startup and harmony resets, and orients the note maps so the first
chord starts there in variation zero. Tests check the first voicing along
15-14-13-12 and chord-root order along 15-11-7-3, alongside all map, reset and
clock regressions. P5 was installed on the same spare with all 26 staging blocks
verified, checksum `1e15` accepted and a fresh `HARMONY5` report confirmed.
Physical confirmation of the corrected orientation is pending.

## Development emulation

Private development tests executed the compiled ARM extension and original
application/loader instructions obtained from the owner's own vendor files.
Those files, extracted instructions and device logs are intentionally excluded
from this public directory. The 54-test development suite covered, among other
cases:

- All 224 maps, scale/chord membership, distinct pitch pairs and two-octave bounds.
- Deterministic startup over stale note data, pot bin centers and hysteresis.
- Existing single-pad playback, both clock handlers, expected pitch codes and
  gates, and preservation of neighboring state and interrupt masks.
- Extended-image mapping and checksum, staged-data corruption, premature packet
  processing and refusal to replay uncertain commits or launch failed transfers.

These were models of selected paths. They did not model analog noise, touch
electronics, real interrupt timing, oscillator tuning, every storage state, all
USB host/controller behavior, or power loss during a write.

## Public tests and CI

Run `python -m unittest discover -s tests -v` from this directory. Fixtures are
synthetic and contain no vendor code. Tests cover container round trips,
manifest matching, packet framing, RAM verification, commit/error handling,
source pinning and CLI help. Native C tests compile only our pure note generator
and selector functions plus the pure tap/clock engine, checking all layouts,
boundary noise and clock scenarios. They run when a
Unix C compiler is present and skip on Windows. A CI matrix covers Windows and
Ubuntu with Python 3.11 and 3.12.

## Scope

Only the inspected legacy revision `001b`, known migration installer, official
Cellz V0.0.3.0 file and locally built Harmony P1 through P5 are supported. Chipz/NiftyCASE
and other migration versions are deliberately outside the public updater's scope.
No tool reads back the complete flash, establishes debug access, changes drivers,
performs automatic rollback, or guarantees recovery from a failed bootloader write.
