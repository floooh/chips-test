"Ok, regarding NTSC-VIC-emulation and reporting of the rasterline in $9004 I
finally wrote a little test program, that waits for $9004 and does a little
effect with $900f. To make this visible on the screen for the interesting
rasterlines near the beginning and end of the frame I included a waiting loop.
There is no adjustment for jitter so you can see the line
happily jittering and see where the earliest switch is recognized. The result
is the same for values 1 to 130 (press SPACE to go through the values):

The effect starts at position 0.

However for value 0 in $9004:

it starts at position 33.

For fun I switched the interlace-bit on with POKE36864,133 before starting this
test and the results are the same for values 1 to 131 (one extra value in inter-
lace). However for value 0 one half-frame starts at position 33 and the other at
position 0 as well.

So it looks like the reporting to $9004 is delayed for 33 cycles when a new
frame starts and not when a half-frame starts. I could not find any other
irregularities regarding reporting of $9004 just the one for value 0."
