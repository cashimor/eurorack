This is a Decay module, with an input and output and a knob that can control it, and a ADSR module with controls for Attack, Decay/Release (same) and Sustain.
The Sustain can only be controlled through a jack, although it does have a trimpot at the back to set the default. The others can be controlled by jack or by
know, where the knob can attentuate the jack as well.

This is a fully functional system. It will require a Microchip programmer with a standard 6 pin header to load the sourcecode. Power capacitors are in place.
Protection resistors are in place. The static protecction diodes are used to protect against overvoltage on the inputs of the PIC16F18115, which pushes the
device slightly out of specification. This shouldn't cause issues in normal use (but does mean one should be wary about using bad voltages in static
environments).
