# Cellz Harmony prototype 4

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

Harmony is our original C extension integrated into a separately obtained Cellz
V0.0.3.0 image. It replaces the note-generation/control behavior while retaining
the original touch scanning, clock/gate handling, pitch conversion and USB support.
This is not a recovered full source tree for the manufacturer's application.

## Controls

| Control | Prototype 4 behavior |
| --- | --- |
| First original ADC/pot channel | First half: 14 scales; second half: 13 chord progressions |
| Second original ADC/pot channel | Select one of 16 variations (placement and, in progression mode, melody choices) |
| Touch pads and external stepping inputs | Original selection/stepping behavior |
| Left Tune button | Tap-clock the original left/row stepping direction |
| Right Tune button | Tap-clock the original right/column stepping direction |
| Multiple touched pads | Existing touch arpeggiation, fixed at 250 ms |

The physical knob direction depends on the board/control wiring. The modified
spare used during development has added CV inputs on the pot circuits and gate
controls across the Tune buttons. Firmware reads the original ADC channels;
those CV modifications can therefore select harmony/layout too. They are not
required to operate the knobs, but an unmodified unit has not been tested here.

In the scale bank, the upper voice uses a full scale, the lower voice its associated chord tones.
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

## Progression bank

The first 2048 decoded ADC counts select the 14 scales above; the remaining 2048
select these 13 progressions, in order. The physical clockwise/counterclockwise
direction depends on the original pot wiring. A 24-count hysteresis band applies
to every selection boundary, including the midpoint between the two banks.

| Progression | Chords with C as home tonic |
| --- | --- |
| I-V-vi-IV | C, G, Am, F |
| ii-V-I-I | Dm, G, C, C |
| I-I-IV-V | C, C, F, G |
| I-vi-IV-V | C, Am, F, G |
| vi-IV-I-V | Am, F, C, G |
| I-vi-ii-V | C, Am, Dm, G |
| IV-V-I-vi | F, G, C, Am |
| I-III-IV-iv | C, E, F, Fm |
| I-bVII-IV-I | C, Bb, F, C |
| i-VII-VI-V | Cm, Bb, Ab, G |
| i-iv-v-i | Cm, Fm, Gm, Cm |
| i-VI-III-VII | Cm, Ab, Eb, Bb |
| iv-III-bII-I | Fm, Eb, Db, C |

Each progression assigns four consecutive groups of four pads in variation zero.
The lower voice plays root, third, fifth, octave for each chord. The four upper notes
are selected from that chord's full major scale for uppercase chords, or full
natural minor scale for lowercase chords. They are **not restricted to the triad**.
Across the variations, all seven scale degrees are available for every chord.
The four pads cannot contain all seven notes simultaneously; the variation
control chooses which four and their octave placement.

These are per-chord scales, rather than one fixed scale for the home key: with a
C tonic, the G chord uses G major, including F#, and the Am chord uses A natural
minor. Borrowed E major and F minor likewise get their own scales. Capitalization
sets chord quality. III/VI/VII in the final four minor-home progressions use the
natural minor degree positions; the final progression explicitly borrows bII and
a major I. Tune both oscillators to establish the home tonic as before.

In progression mode the variation knob changes both the melodic choices and the
paired-pad permutation. Notes are folded into the original two-octave range;
the lower part stays with the chord's triad and octave root. Duplicate note pairs are avoided
even when a progression repeats a chord. A progression is a static pad map:
touch selection and either clock direction travel through it; there is no
separate automatic chord-change sequencer.

## Tap clocks

Both directions start stopped at power-on. Tap a Tune button at least three times
at a steady tempo and release it after each tap. The third release starts that
direction's clock; its first step follows one complete period later. Additional
taps refine its tempo using the most recent four intervals. Every accepted tempo
setting resets both running clocks to one shared cycle boundary. It adds no
immediate tap-triggered step and keeps the current pad selected. Steps already
due from the old schedule are preserved once, so tapping exactly on each beat
does not stall the sequence. A gap longer than four seconds
starts a new three-tap measurement; a running clock continues during measurement.
Intervals that differ by more than 25% restart confirmation rather than accepting
an abrupt inconsistent estimate.

Hold a button for **800 ms** to stop its direction. Release it, then give three
fresh taps to restart. Holding or tapping both buttons handles both independently;
there is no combined-button quantization-mode gesture. Buttons are debounced;
very short external gate pulses across the switches may not register as taps.

The first running direction establishes a tempo. When the other direction is set,
its requested step duration snaps to the nearest permitted ratio of the running
direction: **1:1; 1:2 through 1:8 and their inverses; 2:3, 3:2, 3:4, 4:3**.
The established clock keeps its tempo; both phases restart together. At 120 BPM, tapping the other
button at 40 or 30 BPM yields those tempos; about 35 BPM snaps to 40 BPM. A
120/15 BPM pair is also allowed (8:1). Awkward ratios such as 24:7 are excluded.

Measured tap intervals are 150..4000 ms (400..15 BPM); ratio selection is subject
to that tap range. Fractional
milliseconds accumulate rather than rounding every period to a whole millisecond;
ratio calculations have microsecond precision and steps use the existing 1 ms
main loop. They are not a precision external clock synchronization system.

Internal clocks pend the original clock interrupts, preserving their existing
movement, pitch, indicator and gate paths. External clock inputs remain active
and add steps; disconnect those clocks for a pure tap-tempo pattern. Stopping a
tap clock does not disable external clocking or touch-pad selection. Clock settings
are temporary and are not written to flash. Physical button/direction mapping and
timing under USB load still need further hardware verification.

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
`firmware/cellz_harmony.c`, `firmware/tap_tempo.c`, `firmware/tap_tempo.h` and
`firmware/harmony.ld`.

Outputs go to ignored `build/harmony/`: a full DFU, a contiguous binary, an ELF
extension and `build.json`. The manifest records source/image/container hashes,
checksum, symbols and exact integration patches. Locally generated original-byte
patch records also stay in that ignored directory.

The P4 candidate has image SHA-256
`5744bf8e3e9f35366ae0950b540484e783e2a359d54ac86826772da7c03bd8ca`,
checksum `539a`, and 26 transmitted 1 KB blocks. The extension itself is 2332 bytes.
It changes 18 bytes at six integration sites and adds three application pages.
The SysTick vector now points to a wrapper that preserves the original countdowns
and maintains clock time. Tap state uses a separate 256-byte RAM region above the
pinned application's runtime/stack reservation; it does not reuse its USB buffers.
The builder preserves the transport block at transmitted index 9 and does not
include bootloader or factory metadata programming.

**The generated full DFU contains vendor application code. Keep it local; do not
commit it or attach it to this repository's releases.** Share the source and build
instructions instead.

## Install and verify

Prepare the Windows environment and HID loader as described in
[the updater guide](windows-updater.md). First review the offline plan:

```powershell
python tools/cellz_update.py flash --firmware build/harmony/Cellz_Harmony_P4.UNTESTED.dfu --manifest build/harmony/build.json
```

Add `--execute` to install. The updater verifies the manifest, RAM staging data,
commit acknowledgments and whole-image checksum before starting the application.
It then requires the `HARMONY4` signature. The base USB version remains `0030`;
the signature distinguishes Harmony from the original application.

The `UNTESTED` artifact filename is retained to emphasize that a local rebuild is
experimental and has not itself undergone a hardware trial. P1 was installed on
one modified spare and reported to work excellently. P2's live trial prompted the
shared-cycle improvement. P3 was installed on that spare with all staging blocks
verified, checksum accepted and its application signature confirmed; its musical
behavior still awaits user verification.
P4 has passed offline tests but has not been installed on hardware.
Try pad selection, both stepping directions, both outputs and the two knobs. Check
long power-off/restart behavior when your hardware setup can safely be power-cycled.
For P4, also check the midpoint between banks, all 13 progressions, melody changes
with the variation control, each button/direction, hold-to-stop, restarting, simultaneous
buttons, both 1:4 and 1:8 ratios, tempo changes near step boundaries, continued
tapping on each beat, and operation alongside external clocks.
Restore the separately downloaded official file using the normal flash command
if needed. Neither update nor restore has an automatic rollback or flash backup.
