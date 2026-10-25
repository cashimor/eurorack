# Cellz Drum D3

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

Drum is a separate four-instrument sequencer with two pattern banks, each with
16 or 32 steps. Both banks and all instruments share the playback length.
It sends pulses to an external drum module; it does not generate audio.
Harmony remains a separate application with its own unchanged source and builder.

D1 was installed on one modified spare Cellz on 2026-10-24, with all staging blocks
verified, checksum accepted, and a fresh `DRUM0001` status. **D2 was installed
on that spare on 2026-10-25:** all 28 staging blocks were verified, checksum `b6ef`
was accepted, and a fresh USB read confirmed `DRUM0002` with playback advancing.
D2 added Shift accents, triplet pairs and storage migration. D3 adds selectable
16/32-step playback and four edit pages. **D3 was installed on that spare on
2026-10-25:** all 29 staging blocks were verified, checksum `fcab` was accepted,
and a fresh USB read confirmed `DRUM0003`, 16-step mode, page A1 and playback
advancing. Physical gesture direction, LED visibility, clock timing,
output voltages and power-off restoration need hardware verification.

## Controls

| Control | Function |
| --- | --- |
| Former harmony knob | Internal tempo, 40–240 BPM, four sixteenth steps per beat |
| Former variation knob | Swing, 50% straight to 75% of a two-step pair |
| Left Tune button, short click | Cycle edited instrument 1 → 2 → 3 → 4 → 1 |
| Right Tune button, short click | 16 steps: A1 → B1; 32 steps: A1 → A2 → B1 → B2 → A1 |
| Hold right button + move swing knob appreciably | Up: 32 steps; down: 16 steps |
| Pad without a Shift button | Toggle that step |
| Hold right button + touch a pad | Toggle its accent; CV instruments 3 and 4 only |
| Hold left button + touch one pad | Toggle a triplet covering this step and the next |
| Hold both buttons 1.5 seconds, without pad edits | Save length, both banks, all four instruments and their modifiers |
| Left gate input | External sixteenth-note clock; overrides the internal clock |
| Right gate input | While high, invert the bank selected with the right button |

A Shift edit consumes that button press: release does not change instrument or
bank. There is no need to press two pads together. **Neither Shift gesture can
turn a step off**: it enables the step/pair before editing the modifier. Holding
both buttons together is reserved for saving; pads cannot edit in that state,
and touching a pad cancels saving until both buttons have been released.
A long single-button press without a pad has no selection action.

Ordinary multi-pad touches still toggle every touched step, allowing hi-hat
painting. A held pad does not toggle again when the instrument or bank changes.
There is no start/stop or erase-all gesture in D3; internal playback starts at boot.

## Length and edit pages

Without a save, playback starts at 16 steps. To change it, hold the **right Tune
button**, then move the swing knob upward by about one eighth of its full travel
for **32 steps**, or downward by the same amount for **16 steps**. One top LED
confirms 16; two top LEDs confirm 32 while the button remains held. Release the
button afterward. The gesture consumes the click, so release does not select
another page. Even selecting the current length consumes the click.

This is a relative movement: start with enough knob travel in the desired
direction. Only one length selection is accepted per hold. Pads are ignored
after a length gesture until release. Touching a pad before or during the movement
instead reserves the hold for accent editing; it cannot also change length.
Adding the second button after a length gesture cannot accidentally save.

Swing stays at its previous value while the right button is held. After release,
it stays there until the knob reaches or crosses that previous position. Then
ordinary swing control resumes. This pickup also applies after accent edits or
page clicks; if the knob has not moved, it releases immediately.

Each bank has two 16-pad pages. A1 and B1 edit steps 1–16; A2 and B2 edit steps
17–32. In 32-step mode, short right-button clicks cycle **A1 → A2 → B1 → B2**.
The displayed page stays where you selected it while playback runs through both
halves. The playhead blink appears only when playback is on that page. The right
gate still inverts A/B, keeping the same half and playback position: A2 becomes
B2 while the gate is high, for example. USB inspection reports both selected
and audible/editable page names.

Changing length returns the editor to page 1 of the selected bank, clears pending
hits and pulses, and restarts the sequence from step 1 on the next clock step.
It preserves the tempo, swing and all patterns. Steps 17–32, accents and triplets
remain stored when playing 16 steps; selecting 32 restores access to them.
Fresh defaults and migrated D1/D2 saves have blank second halves, so expanding
does not automatically repeat the first bar. Both pages play all four instruments.

Triplets use the next step at the current length: step 16 pairs with step 1 in
16-step mode, and with step 17 in 32-step mode; step 32 pairs with step 1.
Stored starts are retained on a length change. If the newly adjacent partner is
disabled, the start plays as a single until that partner is enabled. Check pairs
at these boundaries when switching lengths. Only 16 and 32 are selectable in D3.

## Accents

Accents apply only to the **two original CV output jacks**, instruments 3 and 4.
Their normal pulse level uses DAC code 1352 (nominal approximately 3.3 V); an
accent uses code 2048 (nominal approximately 5 V). LOW is code 0. The original
pitch scaling provides the voltage estimate; **D3 jack voltages have not been
measured**. Verify levels before patching voltage-limited inputs.

Hold the right button to view accented pads only. Touch a pad to toggle its accent;
release the button to return to the ordinary step display. A previously disabled
pad becomes enabled. On instruments 1 and 2 this gesture enables the step, but
cannot add a voltage accent because those use fixed digital gate outputs.

An accent on either member of a triplet applies to all three hits. Both pads show
accented in the accent view; touching either while holding the right button toggles
the whole pair's accent. Creating a triplet from an accented single propagates the
accent to both pads.

Suggested Bilma patch: CV instrument 3 → kick, CV instrument 4 → hi-hat. The
updated Bilma code uses lower pulses as normal hits, higher pulses as accents,
and ORs them with its separate accent jack. Accented hi-hat follows Bilma's
original louder open-hat behavior. Plain gate-only drum inputs will simply receive
triggers at either pulse height.

## Triplets

Hold the left button to view triplet pairs only. Touch the **first** pad of a pair:
this step and the next become enabled and play three evenly spaced hits in the
same time normally occupied by two steps. Touch the first pad again with left
Shift to return both pads to normal single steps. The final step of the selected
length pairs with step 1 across the bar boundary. A new overlapping pair replaces the conflicting triplet;
other enabled steps remain enabled.

Triplets can be enabled independently on all four instruments and in both banks.
They keep the bar length unchanged. For example, at an internal tempo of 120 BPM,
two sixteenth steps occupy 250 ms: the three hits are at 0, approximately 83.3 and
166.7 ms within that span. Timing is resolved to the 1 ms firmware tick.

Triplet hits are straight within their two-step span, even with swing enabled;
other instruments retain their own swung single steps. External triplets interpolate
using the latest clock-period estimate. The first external pulse uses the tempo
knob as an estimate until a second clock arrives. Abrupt external period changes
over 25%, clock takeover, timeout, tempo changes or bank changes cancel outstanding
subdivisions. Stalls skip overdue hits instead of replaying a burst. These are
practical clock-following rules, not sample-accurate timing guarantees.

**Turning an enabled pad off normally clears its accent and breaks any triplet
containing it.** The other pad remains enabled as a single step, retaining its own
accent. In the first partial cycle, a wrapping pair's second pad can play as a
single until its first pad has been reached.

## Pads, LEDs, banks and outputs

Step 1 is the physical top (hardware pad 15). Steps follow descending hardware
pad numbers through all 16 positions on each page, then the next page and wrap.
This is a linear sequence rather than Harmony's diagonal movement paths. Normal view lights enabled steps. An
enabled playhead goes dark for 60 ms; Shift views show modifiers without that
playhead blink. The display scans one pad per millisecond (62.5 Hz full refresh),
with one LED electrically active at a time to preserve the original peak load.
Brightness, flicker, camera visibility and physical ordering need bench checks.

All four instruments play while one is edited. The audible/editable bank is
`selected_bank XOR right_gate_high`. In 16-step mode, select A, click to B, raise
the gate to hear A, click to hear B, then lower the gate to hear A. In 32-step
mode it takes two clicks to select the other bank. The view changes immediately;
subsequent hits use that bank, without resetting the playhead. The current 5 ms
pulse can finish normally. Selection starts at instrument 1 and page A1;
a saved length is restored.

| Instrument | Output | Suggested factory sound | Pulse |
| --- | --- | --- | --- |
| 1 | First original gate jack | Snare | Fixed gate, 5 ms |
| 2 | Second original gate jack | Clap | Fixed gate, 5 ms |
| 3 | First original CV jack | Kick | Normal/accent DC level, 5 ms |
| 4 | Second original CV jack | Hi-hat | Normal/accent DC level, 5 ms |

No jack changes direction; PWM is confined to the existing LED circuitry.
Factory defaults load only without a valid save. All modifiers initially are off:

| Instrument | Bank A enabled steps | Bank B enabled steps |
| --- | --- | --- |
| 1 / snare | 5, 13 | 5, 13, 14, 15, 16 |
| 2 / clap | 7, 15 | 3, 7, 11, 15, 16 |
| 3 / kick | 1, 5, 9, 13 | 1, 5, 7, 9, 13 |
| 4 / hi-hat | 1, 3, 5, 7, 9, 11, 13, 15 | 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16 |

Existing D1 saves retain their original pattern-to-output assignments. They are
not reordered to match these suggested sounds. Check your patch and edited
instrument; the firmware does not impose drum names.

## External clock and swing

Left gate rising edges are sixteenth-note steps, with edges less than 20 ms apart
rejected. An edge takes over from the internal clock. After more than three
measured periods without another edge (at least 500 ms), internal timing resumes
from the next step with a fresh phase. Gaps above four seconds use the knob interval
as the new estimate. The tempo knob always sets the internal fallback rate.

Internal swing alternates longer/shorter intervals while preserving pair duration.
External swing delays even-numbered steps (2, 4, …) by up to half a clock period;
feed a straight external clock for this mode. A new edge cancels an overdue delayed
step, avoiding a burst. Long main-loop stalls advance at most one base step per
update. Each output has an independent pulse-expiry timer for triplet playback.

## Saving, D1/D2 migration and power loss

**Hold both buttons for 1.5 seconds with no pad edits to explicitly save everything:**
all 32-step masks, accent masks, triplet starts and playback length for both banks
and all four tracks, including the hidden second halves in 16-step mode.
Release both buttons afterward. Ordinary edits do not write flash. Unsaved edits
are lost at power-off. Saved patterns load independently of the power-off duration;
instrument selection, bank selection, playhead and knob positions are temporary.

Saving briefly mutes outputs, blanks LEDs and pauses interrupts/timing/USB while
programming flash. Save between performances. The brief blanking is not proof of
success; read-only inspection reports `unsaved_edits` and `last_save`:

```powershell
python tools/cellz_update.py inspect
```

Storage remains at `0x0800e800` and `0x0800ec00`, two 1 KB pages outside the pinned
application, loader erase span, bootloader and factory area. D3 uses 120-byte
records (eight per page), containing sequence, 24 32-bit masks, length, CRC32,
format and a commit word written last. Page rotation erases only the page not
holding the latest complete record. Interrupted writes leave the preceding
complete record available.

D3 reads D1's 36-byte and D2's 68-byte records. Boot widens their step/modifier
masks, initializes missing modifiers and second halves to zero, and sets length
16, without a flash write. The first explicit D3 save writes format 3 on the other
page, retaining the old save until the new record is committed. **D1 and D2 cannot
read D3 records.** Later rotations may erase the old record; downgrading firmware
does not roll back pattern data. Keep a written copy of important patterns; USB
pattern import/export is not implemented.

Without any valid record, first use requires an entirely erased page; unidentified
data is not automatically erased. Flash has finite endurance. Failure simulations
cover interrupted programming, migration and erasure; physical power-loss behavior
still needs testing. The known D2/D3 and Harmony P5 application update paths preserve
these pages in offline loader simulation. Other images, mass erase, migration or
recovery procedures may not. There is no automatic firmware rollback.

## Build and install

Obtain the official Cellz V0.0.3.0 file and Zig 0.14.1 separately as described in
[the updater guide](windows-updater.md). From the `cre8audio` directory:

```powershell
python tools/build_drum.py --base-firmware "vendor-input/NiftyBUNDLE Firmware/Step2 - Firmware/Cellz_V0.0.3.0_220407A.dfu" --zig "C:/tools/zig-0.14.1/zig.exe"
python tools/cellz_update.py flash --firmware build/drum/Cellz_Drum_D3.UNTESTED.dfu --manifest build/drum/build.json
```

The second command is an offline plan. Add `--execute` for installation through
the existing Windows/HID setup. The updater requires verified staging, whole-image
checksum and signature `DRUM0003`. Base USB version `0030` is preserved. The builder
refuses an image whose loader erase span reaches pattern storage; it does not
modify the bootloader.

Generated DFU/bin/ELF files contain vendor application code. Keep them local and
never commit or redistribute them. Only our source, tools, tests and documentation
are published. To return to Harmony, use its separate [build/install guide](harmony.md).

For the D3 bench trial, check length gestures in both directions, swing pickup,
the four page cycle, right-gate inversion on each half, 32-step playback and
preserved hidden steps. Also check CV voltage levels before patching the Bilma,
Shift edits, modifier views, cross-page/wrapping triplets, simultaneous tracks,
external clocks/swing, D2 save migration, explicit save and power-off restoration.
