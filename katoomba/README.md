Katoomba is a triple LFO. The first LFO has a -5-5V triangle output, a 0-5V square output and a -5-5V square output.
The second is the slowest, a -5-5V output only. The third is the fastest with a -5-5V triangle output that can be
attenuated and a 0-5V output.

This is a functioning prototype:
* The attentuation potentiometer is reversed (left is open, right is closed)
* There are no power capacitors
* There are no protection resistors on the outputs of the OPAMP
* The circuit has an inherent risk of latching to +12V or -12V when the LFO is set too slow
