# pidaydrummachine.py - Pi Day Drum Machine
# 14 Mar 2021 - @todbot / Tod Kurt
# 29 Jul 2024 - @cashimor / Maarten Hofman: Eurorack

# samples from https://freesound.org -- License: Creative Commons 0
# samples set 1 from: Trvth (Snare1Drum, PedalHiHat1, Ride1Drum, HiHatOpen1Drum, Bass1Drum, Crash1Drum)
# samples set 2 from: BaDoink (Drum Kit Pack 1), DigitalUnderglow (drumhit_Clap3)
# samples set 3 from: RytmenPinnen (SemiLoopDrumsamples), ankealtd (Meinl_Byzance_16_Extra_Dry_Thin_Crash_H_3)
# samples set 4 from: IanStarGem (Electronic Snare Drum 4, Simple Kick Drum, Electronic Closed Hihat #2, Electronic Snare Drum #3)
# additional samples from: pomeroyjoshua (Anu Clap 06, HH Op 04)

import time
import board
import keypad
import array
import audiocore
import audiomixer
import analogio
import math
import random
from audiopwmio import PWMAudioOut as AudioOut

wav_files = (
    "wav/bass/",
    "wav/snare/",
    "wav/clap/",
    "wav/open/",
    "wav/closed/",
)

def metalwave(length, decay, p1, p2, p3, p4, p5, n):
    noise = array.array("h", [0] * length)
    tone_volume = 1
    c1 = 0
    c2 = 0
    c3 = 0
    c4 = 0
    c5 = 0
    osc1 = -32767
    osc2 = -32767
    osc3 = -32767
    osc4 = -32767
    osc5 = -32767
    noise = array.array("h", [0] * length)
    for i in range(0, length):
        c1 = c1 + 1
        if c1 > p1:
            c1 = 0
            osc1 = -osc1
        c2 = c2 + 1
        if c2 > p2:
            c2 = 0
            osc2 = -osc2
        c3 = c3 + 1
        if c3 > p3:
            c3 = 0
            osc3 = -osc3
        c4 = c4 + 1
        if c4 > p4:
            c4 = 0
            osc4 = -osc4
        c5 = c5 + 1
        if c5 > p5:
            c5 = 0
            osc5 = -osc5
        value = (osc1 + osc2 + osc3 + osc4 + osc5) / 6
        value = (value + int(random.random() * n * 32767)) / 2
        noise[i] = int(value * tone_volume)
        tone_volume = tone_volume * decay
    return noise

def sinewave(length, decay, pitch):
    tone_volume = 1
    sine_wave = array.array("h", [0] * length)
    for i in range(length):
        sine_wave[i] = int(math.sin(math.pi * 2 * i / (pitch + (i/75))) * tone_volume * 32767)
        tone_volume = tone_volume * decay
    return sine_wave

def noisewave(length, decay, pitch):
    tone_volume = 1
    noise = array.array("h", [0] * length)
    count = 0
    value = random.random()
    for i in range(0, length):
        count = count + 1
        if count > pitch:
            count = 0
            value = random.random()
        noise[i] = int(value * tone_volume * 32767)
        tone_volume = tone_volume * decay
    return noise

def alternate(drums, select):
    if drums == 0:
        select = select + 1
        if select > 7:
            select = 0
    else:
        drums = drums + 1
        if drums > 4:
            drums = 1
    return drums, select

# what keys to use as our drum machine
keys = keypad.Keys((board.GP16, board.GP17, board.GP18, board.GP19, board.GP20),
                   value_when_pressed=True, pull=True)

audio = AudioOut(board.GP15)
mixer = audiomixer.Mixer(voice_count=10, sample_rate=22050, channel_count=1,
                         bits_per_sample=16, samples_signed=True)
audio.play(mixer) # attach mixer to audio playback, play with mixer.voice[n].play()
adc = analogio.AnalogIn(board.A2)
drums = "1"

bass1 = audiocore.RawSample(sinewave(10000, 0.9995, 150), channel_count = 1, sample_rate = 22050)
bass2 = audiocore.RawSample(sinewave(10000, 0.9998, 160), channel_count = 1, sample_rate = 22050)
# bass3 = audiocore.RawSample(sinewave(10000, 0.9996, 130), channel_count = 1, sample_rate = 22050)
openhh = audiocore.RawSample(metalwave(10000, 0.9997, 45, 87, 353, 452, 493, 0.8), channel_count = 1, sample_rate = 22050)
snare1 = audiocore.RawSample(noisewave(5000, 0.9993, 0), channel_count = 1, sample_rate = 22050)
snare2 = audiocore.RawSample(noisewave(5000, 0.9992, 3), channel_count = 1, sample_rate = 22050)
clap = audiocore.RawSample(noisewave(2500, 0.998, 4), channel_count = 1, sample_rate = 22050)
hihat1 = audiocore.RawSample(metalwave(2500, 0.9985, 3, 13, 133, 251, 333, 0), channel_count = 1, sample_rate = 22050)
hihat2 = audiocore.RawSample(metalwave(2500, 0.9989, 9, 77, 5, 249, 363, 0.5), channel_count = 1, sample_rate = 22050)

c1 = ( bass1, snare1, clap, hihat1, openhh )
c2 = ( bass2, snare1, clap, hihat1, openhh )
c3 = ( bass1, snare2, clap, hihat1, openhh )
c4 = ( bass2, snare2, clap, hihat1, openhh )
c5 = ( bass1, snare1, clap, hihat2, openhh )
c6 = ( bass2, snare1, clap, hihat2, openhh )
c7 = ( bass1, snare2, clap, hihat2, openhh )
c8 = ( bass2, snare2, clap, hihat2, openhh )

combo = ( c1, c2, c3, c4, c5, c6, c7, c8 )

while True:
  reading = adc.value
  select = reading & 57344;
  if select == 32768:
      drums = 1
  elif select == 40960:
      drums = 2
  elif select == 49152:
      drums = 3
  elif select == 57344:
      drums = 4
  else:
      drums = 0
      select = int(reading / 4096)
  swap = reading & 512
  event = keys.events.get()
  if event and event.pressed:
      n = event.key_number
      level = 1
      print(select, swap)
      if swap:
          if n == 2:
              drums, select = alternate(drums, select)
              n = 1
          elif n == 4:
              drums, select = alternate(drums, select)
              n = 0
      if drums > 0:
          wave = audiocore.WaveFile(open(wav_files[n] + str(drums) + ".wav", "rb"))
      else:
          wave = combo[select][n]
      mixer.voice[n].play(wave)
