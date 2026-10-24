# Cellz Harmony prototype 1

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

Harmony is our original C extension integrated into a separately obtained Cellz
V0.0.3.0 image. It replaces the note-generation/control behavior while retaining
the original touch scanning, clock/gate handling, pitch conversion and USB support.
This is not a recovered full source tree for the manufacturer's application.

## Controls

| Control | Prototype 1 behavior |
| --- | --- |
| First original ADC/pot channel | Select one of 14 shared harmonic settings |
| Second original ADC/pot channel | Select one of 16 paired layouts |
| Touch pads and external stepping inputs | Original selection/stepping behavior |
| Front-panel Tune buttons | Inactive; no editing or quantization-mode shortcut |
| Multiple touched pads | Existing touch arpeggiation, fixed at 250 ms |

The physical knob direction depends on the board/control wiring. The modified
spare used during development has added CV inputs on the pot circuits and gate
controls across the Tune buttons. Firmware reads the original ADC channels;
those CV modifications can therefore select harmony/layout too. They are not
required to operate the knobs, but an unmodified unit has not been tested here.

The upper voice uses a full scale, the lower voice its associated chord tones.
The lower part repeats roots more often and occasionally uses thirds, fifths and,
for seventh-chord settings, sevenths. Each voice spans 0..24 semitones relative to
its reference, retaining the manufacturer's two-octave reference difference.
Tune your oscillators to establish the desired root/key.

No two pads share the entire pitch pair. Individual notes and pitch classes can
repeat. Layouts are deterministic rotations/permutations of paired material,
not random notes. Each startup regenerates a useful map independently of old
stored notes. The existing ADC averaging plus 24-count selector hysteresis avoids
boundary chatter; inside a hysteresis band, the previous adjacent setting can be
retained until the control moves farther across the boundary.

| Setting | Upper scale (semitones from root) | Lower chord |
| --- | --- | --- |
| Major / Ionian | 0 2 4 5 7 9 11 | 0 4 7 |
| Major7 | 0 2 4 5 7 9 11 | 0 4 7 11 |
| Minor / Aeolian | 0 2 3 5 7 8 10 | 0 3 7 |
| Minor7 | 0 2 3 5 7 8 10 | 0 3 7 10 |
| Harmonic minor | 0 2 3 5 7 8 11 | 0 3 7 |
| Melodic minor (fixed ascending form) | 0 2 3 5 7 9 11 | 0 3 7 |
| Major pentatonic | 0 2 4 7 9 | 0 4 7 |
| Minor pentatonic | 0 3 5 7 10 | 0 3 7 |
| Minor blues | 0 3 5 6 7 10 | 0 3 7 |
| Dorian | 0 2 3 5 7 9 10 | 0 3 7 |
| Phrygian | 0 1 3 5 7 8 10 | 0 3 7 |
| Lydian | 0 2 4 6 7 9 11 | 0 4 7 |
| Mixolydian | 0 2 4 5 7 9 10 | 0 4 7 |
| Locrian | 0 1 3 5 6 8 10 | 0 3 6 |

Tap-tempo clocks using the Tune buttons are a future idea, not part of prototype 1.

## Build locally; no complete firmware is distributed

Obtain `Cellz_V0.0.3.0_220407A.dfu` from
[Cre8audio](https://www.cre8audio.com/firmware-and-software) yourself. Use the exact
file fingerprint listed in [the updater guide](windows-updater.md). Obtain Zig
**0.14.1** separately from [the official Zig downloads](https://ziglang.org/download/).
The Windows x86_64 compiler archive used during development has SHA-256
`554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c`.

From this directory, adjusting the paths to your own downloads:

```powershell
python tools/build_harmony.py --base-firmware "vendor-input/NiftyBUNDLE Firmware/Step2 - Firmware/Cellz_V0.0.3.0_220407A.dfu" --zig "C:/tools/zig-0.14.1/zig.exe"
```

If Zig 0.14.1 is on PATH, omit `--zig`. Python performs the container work and
invokes Zig's freestanding ARM C compiler. It does not access USB. Sources are
`firmware/cellz_harmony.c` and `firmware/harmony.ld`.

Outputs go to ignored `build/harmony/`: a full DFU, a contiguous binary, an ELF
extension and `build.json`. The manifest records source/image/container hashes,
checksum, symbols and exact integration patches. Locally generated original-byte
patch records also stay in that ignored directory.

The reproduced prototype has image SHA-256
`a5c34bbd3aef8f65b33f28bbc40c425443590faf9ecd853da8f8ba380f4dc56a`,
checksum `f6dd`, and 24 transmitted 1 KB blocks. The extension itself is 852 bytes.
It changes 14 bytes at five integration sites and adds one application page.
The builder preserves the transport block at transmitted index 9 and does not
include bootloader or factory metadata programming.

**The generated full DFU contains vendor application code. Keep it local; do not
commit it or attach it to this repository's releases.** Share the source and build
instructions instead.

## Install and verify

Prepare the Windows environment and HID loader as described in
[the updater guide](windows-updater.md). First review the offline plan:

```powershell
python tools/cellz_update.py flash --firmware build/harmony/Cellz_Harmony_P1.UNTESTED.dfu --manifest build/harmony/build.json
```

Add `--execute` to install. The updater verifies the manifest, RAM staging data,
commit acknowledgments and whole-image checksum before starting the application.
It then requires the `HARMONY1` signature. The base USB version remains `0030`;
the signature distinguishes Harmony from the original application.

The `UNTESTED` artifact filename is retained to emphasize that a local rebuild is
experimental and has not itself undergone a hardware trial. The exact prototype
image was installed on one modified spare and reported to work excellently.
Try pad selection, both stepping directions, both outputs and the two knobs. Check
long power-off/restart behavior when your hardware setup can safely be power-cycled.
Restore the separately downloaded official file using the normal flash command
if needed. Neither update nor restore has an automatic rollback or flash backup.
