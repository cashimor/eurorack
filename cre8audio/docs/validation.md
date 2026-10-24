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

The source-only public builder reproduced the exact installed image hash. The
packaged tool refused to proceed when no unique device was connected during the
publication check. Its write paths use the tested host protocol but remain a
portable refactoring that has not itself performed a live update.
Do not treat successful enumeration as proof of recoverability or flash contents.

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
and selector functions, checking all layouts and boundary noise. They run when a
Unix C compiler is present and skip on Windows. A CI matrix covers Windows and
Ubuntu with Python 3.11 and 3.12.

## Scope

Only the inspected legacy revision `001b`, known migration installer, official
Cellz V0.0.3.0 file and locally built Harmony P1 are supported. Chipz/NiftyCASE
and other migration versions are deliberately outside the public updater's scope.
No tool reads back the complete flash, establishes debug access, changes drivers,
performs automatic rollback, or guarantees recovery from a failed bootloader write.
