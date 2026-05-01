# pidaydrummachine.py - Pi Day Drum Machine
# 14 Mar 2021 - @todbot / Tod Kurt
# 29 Jul 2024 - @cashimor / Maarten Hofman: Eurorack
# 12 Apr 2026 - @cashimor / Improve switching
# 01 May 2026 - @cashimor / Load samples again

import array
import time
import board
import audiocore
import audiomixer
import analogio
import digitalio
import os
from audiopwmio import PWMAudioOut as AudioOut

# --- Configuration ---
SAMPLE_RATE = 22050
# Arrays to hold the raw audio data
# We size these to the largest expected sample (e.g., 10000 for long kicks/open hats)
kick_buf  = array.array("h", [0] * 10000) # 10k samples * 2 bytes (16-bit)
snare_buf = array.array("h", [0] * 5000)
clap_buf  = array.array("h", [0] * 2500)
hh_buf    = array.array("h", [0] * 2500)
ohh_buf   = array.array("h", [0] * 10000)


# The 16 "Drum Kits" defined by your matrix (Kick, Snare, Clap, HH, OHH)
DRUM_KITS = [
    (1,1,1,1,1), (1,1,2,2,3), (1,2,3,1,2), (1,3,1,2,1),
    (1,3,2,1,3), (2,1,3,2,1), (2,1,1,3,3), (2,2,2,2,2),
    (2,3,3,1,1), (2,3,1,1,2), (3,1,2,3,2), (3,1,3,2,2),
    (3,2,1,1,2), (3,2,2,1,1), (3,3,3,1,2), (3,3,3,3,3)
]

def load_raw_to_buffer(filename, buffer):
    """Loads a .raw file into a buffer and zeros out the remainder."""
    # Zero out the buffer first to clear old sample remnants
    for i in range(len(buffer)):
        buffer[i] = 0

    try:
        with open(f"/raw/{filename}", "rb") as f:
            f.readinto(buffer)
    except OSError:
        print(f"File /raw/{filename} not found!")

# --- Hardware Setup ---
pin_ids = (board.GP16, board.GP17, board.GP18, board.GP19, board.GP20)
keys = []
for pin in pin_ids:
    key = digitalio.DigitalInOut(pin)
    key.direction = digitalio.Direction.INPUT
    key.pull = digitalio.Pull.DOWN
    keys.append(key)

last_states = [False] * len(keys)
adc = analogio.AnalogIn(board.A2)

audio = AudioOut(board.GP15)
mixer = audiomixer.Mixer(voice_count=5, sample_rate=22050, channel_count=1,
                         bits_per_sample=16, samples_signed=True)
audio.play(mixer)

load_raw_to_buffer(f"kick1.raw", kick_buf)
load_raw_to_buffer(f"snare1.raw", snare_buf)
load_raw_to_buffer(f"clap1.raw", clap_buf)
load_raw_to_buffer(f"hh1.raw", hh_buf)
load_raw_to_buffer(f"ohh1.raw", ohh_buf)

# Create RawSample objects pointing to our buffers
sample_kick  = audiocore.RawSample(kick_buf,  channel_count = 1, sample_rate=22050)
sample_snare = audiocore.RawSample(snare_buf, channel_count = 1, sample_rate=22050)
sample_clap  = audiocore.RawSample(clap_buf,  channel_count = 1, sample_rate=22050)
sample_hh    = audiocore.RawSample(hh_buf,    channel_count = 1, sample_rate=22050)
sample_ohh   = audiocore.RawSample(ohh_buf,   channel_count = 1, sample_rate=22050)

samples = (sample_kick, sample_snare, sample_clap, sample_hh, sample_ohh)

current_kit_index = -1
ro = -10

while True:
    # 1. Check Knob for Kit Selection
    reading = adc.value
    rotation = reading // 2048;
    if abs(rotation - ro) > 1:
      ro = rotation
      select = reading // 4096 # 0 to 15
      if select != current_kit_index:
        current_kit_index = select
        k, s, c, h, o = DRUM_KITS[select]
        print(f"Loading Kit {select}: K{k} S{s} C{c} H{h} O{o}")

        # Load the files into the buffers
        load_raw_to_buffer(f"kick{k}.raw", kick_buf)
        load_raw_to_buffer(f"snare{s}.raw", snare_buf)
        load_raw_to_buffer(f"clap{c}.raw", clap_buf)
        load_raw_to_buffer(f"hh{h}.raw", hh_buf)
        load_raw_to_buffer(f"ohh{o}.raw", ohh_buf)

    # 2. Key Handling
    is_accented = keys[4].value

    for i in range(5):
      current_state = keys[i].value
      # Detect the "Rising Edge" (Transition from False to True)
      if current_state and not last_states[i]:
        if i == 4:
          for j, key in enumerate(keys):
            if j == 4: continue
            if key.value:
              mixer.voice[j].level = 1.0
              if j == 3 and not last_states[4]:
                last_states[4] = current_state
                wave = samples[4]
                mixer.voice[3].play(wave)
          continue
        # Trigger your drum sound here
        wave = samples[i]
        velocity = 1.0 if is_accented else 0.5
        if i == 3: # Hi-Hat logic
          if is_accented:
            wave = samples[4]
        mixer.voice[i].level = velocity
        mixer.voice[i].play(wave)
      # Update the memory for the next loop iteration
      last_states[i] = current_state
