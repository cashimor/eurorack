Warrima is a 2-pole filter module. The two inputs are either a high pass or low pass filter. The circuit and schematic are a bit of a mess as there was an attempt to mix a VCA and a VCF.

The "Filter_2" gerber is the panel for the warrima filter, but also for the VCA.

Be aware of the following issues:
* No ground plane
* No decoupling capacitors
* The TL074 is reversed (so do not mount it as drawn on the circuit board)
* The transistors are reversed (so do not mount them as drawn on the circuit board)
* The route of the high pass filter is missing
* One of the OPAMP should be properly grounded so it doesn't start using unreasonable power
