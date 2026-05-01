Bilma is a Raspberry Pi pico based drum module. It features 16 sets, loaded from 3 different raw samples. It has a dial
to select the kit, and a dial to change the output filter, which for some reason has some overdrive. The python
file works, and the gerber file and schematic work too.

Original file: https://github.com/todbot/circuitpython-tricks/blob/main/larger-tricks/pidaydrummachine.py

The following samples are still stored here, but are no longer used, so the license doesn't apply if these aren't downloaded:
samples from https://freesound.org -- License: Creative Commons 0
samples set 1 from: Trvth (Snare1Drum, PedalHiHat1, Ride1Drum, HiHatOpen1Drum, Bass1Drum, Crash1Drum)
samples set 2 from: BaDoink (Drum Kit Pack 1), DigitalUnderglow (drumhit_Clap3)
samples set 3 from: RytmenPinnen (SemiLoopDrumsamples), ankealtd (Meinl_Byzance_16_Extra_Dry_Thin_Crash_H_3)
samples set 4 from: IanStarGem (Electronic Snare Drum 4, Simple Kick Drum, Electronic Closed Hihat #2, Electronic Snare Drum #3)
additional samples from: pomeroyjoshua (Anu Clap 06, HH Op 04)

This is a tested circuit. The PCB and schematic will work as included in this repository. Bugs:
Access to the USB connector is a bit cramped after placing the PICO on the board, but it would work with a 90 degree connector.

The "beat.c" file is for a 16F684 to add a drum sequencer.
