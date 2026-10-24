# Cellz Drum D1

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

D1 is a separate experimental drum sequencer application. It preserves the
Harmony source and builder; installing either application replaces the running
application. D1 has passed host and compiled ARM simulation tests, but has not
been installed or tested electrically on a Cellz. LED brightness/ghosting, touch
behavior while scanning, trigger timing and output voltages need a hardware trial.
It does not generate audio: connect its four pulse outputs to drum trigger inputs.

## Controls

| Control | Drum function |
| --- | --- |
| Former harmony knob | Internal tempo, 40–240 BPM; four sixteenth-note steps per beat |
| Former variation knob | Swing, straight 50% to 75% of each two-step pair |
| Left Tune button, short click | Cycle instrument 1 → 2 → 3 → 4 → 1 |
| Right Tune button, short click | Toggle the selected pattern bank A/B |
| Pad | Toggle that step of the displayed instrument in the currently audible bank |
| Left gate input | External clock: one rising jack edge per sixteenth-note step |
| Right gate input | While high, invert the selected bank |
| Both Tune buttons held 1.5 seconds | Save all eight patterns to flash |

The bank playing and being edited is `selected_bank XOR right_gate_high`.
For example: select A, click to B, raise the gate to hear A, click to hear B,
then lower the gate to hear A. Gate changes and bank clicks take effect on the
next step; the LED/edit view changes immediately. They do not reset the playhead.
A pad already held does not toggle again when you change instruments or banks.
Brief simultaneous button presses do not select an instrument or bank; release
both before another click. A long single-button press has no action in D1.

All four instruments keep playing while you edit one. D1 starts with instrument
1 and bank A selected, and the internal clock runs automatically. It has no
start/stop or erase-all gesture yet. There is no dedicated bank/instrument LED;
count the clicks from startup, or use the updater's read-only `inspect` status.

## Pads, LEDs and outputs

Step 1 is the physical top (hardware pad 15). Steps follow descending hardware
pad numbers through all 16 positions and wrap. This is a linear 16-step sequence,
not Harmony's two diagonal movement paths. Verify the physical order in the first
hardware trial. Enabled steps are lit; disabled steps stay dark. An enabled
playhead briefly goes dark for 20 ms as playback passes it. The display scans one
pad per millisecond (62.5 Hz full refresh), with only one LED electrically on at
a time, preserving the original peak LED load. It may appear dimmer or flicker;
brightness and camera appearance need a bench check.

| Instrument | Output jack | Pulse |
| --- | --- | --- |
| 1 | First original gate output | 5 ms, existing digital gate circuit |
| 2 | Second original gate output | 5 ms, existing digital gate circuit |
| 3 | First original CV output | 5 ms, DAC code 0 → 2048 → 0 |
| 4 | Second original CV output | 5 ms, DAC code 0 → 2048 → 0 |

The CV high setting is inferred to be approximately 5 V from the original pitch
scaling. **Neither CV low nor high voltage has been measured for D1.** Measure
the jacks before connecting them to an ADC or other voltage-limited input.
D1 uses DC levels on the CV jacks and ordinary digital pulses on the gate jacks;
its PWM is confined to the existing LED circuitry. No jack changes direction.

Factory patterns below list enabled steps (numbered 1–16). They load only when
there is no valid saved record. All four patterns in each bank play together.

| Instrument | Bank A | Bank B |
| --- | --- | --- |
| 1 | 1, 5, 9, 13 | 1, 5, 7, 9, 13 |
| 2 | 5, 13 | 5, 13, 14, 15, 16 |
| 3 | 1, 3, 5, 7, 9, 11, 13, 15 | 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16 |
| 4 | 7, 15 | 3, 7, 11, 15, 16 |

## External clock and swing

An accepted left input edge immediately takes over from the internal clock.
External pulses are steps, not quarter notes; use a 16th-note clock. Edges less
than 20 ms apart are rejected. The tempo knob continues to set the fallback
internal rate. If pulses stop for more than three measured periods (at least
500 ms), the internal clock resumes from the next step with a fresh timing phase.
Gaps above four seconds use the knob interval as the new period estimate.

Swing keeps an internal two-step pair's total duration constant. With an external
clock, even-numbered steps (2, 4, …) are delayed by up to half the measured pulse
period. Feed a straight external clock if you want D1 to apply swing. A new input
edge cancels an overdue delayed step instead of replaying a burst. Long main-loop
stalls also advance at most one internal step per update. Hardware timing uses a
1 ms tick, so it is not sample-accurate.

## Saving and power loss

**Hold both Tune buttons for 1.5 seconds to save both banks and all four tracks.**
Release both afterward. The LEDs briefly blank during feedback. Ordinary pad
edits do not write flash. Unsaved edits disappear at power-off; saved patterns
are loaded at boot independently of how long power was absent. The selected
instrument/bank, playhead and knob positions are not stored.

The save briefly mutes every output, blanks the LEDs, disables interrupts and
pauses timing/USB handling while programming flash. Save between performances.
Read-only inspection reports `unsaved_edits` and `last_save` (`none`, `success`,
`failed`). Inspect after the save finishes to confirm success; the brief blanking
alone is not a success indication.

```powershell
python tools/cellz_update.py inspect
```

Patterns use two 1 KB flash pages at `0x0800e800` and `0x0800ec00`, outside the
pinned application, its update erase span, bootloader and factory settings area.
Each page holds 28 records. Each save appends a sequence number, eight pattern
masks, CRC32, format version and a commit word written last. When full, only the
other page is erased. Interrupted writes are ignored, leaving the preceding
complete save available. First use requires an entirely erased page: unidentified
data is not automatically erased. Flash has finite endurance; the explicit save
gesture avoids programming it for every edit. Physical power-loss behavior still
needs hardware validation; the failure tests model flash programming and erasure.

D1 and the current small Harmony P5 update fit below these pages, including the
loader's extra erased transport page. Their update paths were checked offline.
This is not a guarantee for other applications, larger future images, mass erase,
bootloader migration, debugger recovery, or a different loader. There is no USB
pattern import/export feature in D1. Keep a written copy of important patterns.

## Build and install

Obtain the official Cellz V0.0.3.0 file and Zig 0.14.1 separately as described in
[the updater guide](windows-updater.md). From the `cre8audio` directory:

```powershell
python tools/build_drum.py --base-firmware "vendor-input/NiftyBUNDLE Firmware/Step2 - Firmware/Cellz_V0.0.3.0_220407A.dfu" --zig "C:/tools/zig-0.14.1/zig.exe"
python tools/cellz_update.py flash --firmware build/drum/Cellz_Drum_D1.UNTESTED.dfu --manifest build/drum/build.json
```

The second command is an offline plan. Add `--execute` for an actual installation
using the existing Windows/HID setup. Success requires verified staging, the
whole-image checksum, and the application signature `DRUM0001`; base USB version
`0030` is preserved. The builder refuses an image whose loader erase span reaches
pattern storage. The bootloader is not modified.

Generated DFU/bin/ELF files contain vendor application code and must stay local.
Do not commit or redistribute them. Only our extension sources, tests and tools
are published. D1 sources are `cellz_drum.c`, `drum_engine.c/.h`, `drum_store.c/.h`
and `drum.ld`; `build_drum.py` builds them independently from Harmony.

To return to Harmony, use its existing [build/install commands](harmony.md) and
manifest. The Harmony source and builder are unchanged. Switching applications
replaces the application; it does not run both at once or provide automatic rollback.

For the first bench trial: verify four output levels and pulse widths, sixteen-pad
LED scanning with a dense pattern, every pad's physical order and reliable toggling,
both button mappings, bank inversion, external clock takeover/fallback, swing,
USB inspection, explicit save, then power-off restoration. Leave the RP2350 ADC
connections out until their input scaling and the Cellz CV levels are measured.
