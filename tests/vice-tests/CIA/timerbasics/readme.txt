
This directory contains some timer tests based on test programs provided by
Andre Fachat.

test.s:

sets the timer to underflow at 15 clk pulses, while the read loop takes 
14 clks only. This gives characteristic read pattern in the ICR.

timer.s:

timer_test1.s:
