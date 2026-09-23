10 February 2013 - https://www.linusakesson.net/scene/safevsp/index.php

the dreaded vsp crash is caused by a metastability condition in the dram. some 
have speculated that it has to do with refresh cycles, but hopefully the 
detailed explanation in this scroller will crush that myth once and for all. 

but first, this is what the machine behaves like from a programmer's point of 
view. let us call memory locations ending in 7 or f fragile. sometimes when vsp 
is performed, several fragile memory cells are randomly corrupted according to 
the following rule: each bit in a fragile memory cell might be changed into the 
corresponding bit of another fragile cell within the same page. this specific 
behaviour can be exploited in several ways: one approach is to ensure that every 
fragile byte in a page is identical. if the page contains code, for instance, 
corruption is avoided if all the fragile bytes are $ea (nop). similarly, in font 
definitions, the bottom line of each character could be blank. another technique 
is to simply avoid all fragile memory locations. the undocumented opcode $80 
(nop immediate) can be used to skip them. data structures can be designed to 
have gaps in the critical places. this latter technique is used in this demo, 
including the music player of course. data that cannot have gaps, i.e. graphics, 
is continuously restored from safe copies elsewhere in memory. you can use shift 
lock to disable this repair, and eventually you should see garbage accumulating 
on the screen. and yet the code will keep running. thus, for the first time, the 
vsp crash has been tamed. 

now for the explanation. the c64 accesses memory twice in every clock cycle. 
each memory access begins with the lsb of the address (also known as the row 
address) being placed on an internal bus connected to the dram chips. as soon as 
the row address is stable, the row address strobe (ras) signal is given. each 
dram chip now latches the row address into a register, and this register 
controls a multiplexer which connects the selected memory row to a set of wires 
called sense lines. each sense line connects to a single bit of memory. the 
sense lines have been precharged to a voltage in between logical zero and 
logical one. the charge stored in the memory cell affects the sense line towards 
a slightly lower or higher voltage depending on the bit value. a feedback 
amplifier senses the voltage difference and exaggerates it, so that the sense 
line reaches the proper voltage representing either zero or one. because the 
memory cell is connected (through the multiplexer) to the sense line, the 
amplified charge will also flow back and refresh the memory cell. hence, a 
memory row is refreshed whenever it is opened. 

vsp is achieved by triggering a badline condition during idle mode in the 
visible part of a rasterline. when this happens, the vic chip gets confused 
about what memory address to access during the half-cycle following the write to 
$d011. it sets the internal bus lines to 11111111 in preparation for an idle 
fetch, but suddenly changes its mind and tries to read from an address with an 
lsb of 00000111. now, since electrical lines can't change voltage 
instantaneously, there is a brief moment of time when each of the changing bits 
(bit 3 through 7) is neither a valid one nor a valid zero. but because the vic 
chip changes the address at an abnormal time, there is now a risk that the ras 
signal, which is generated independently by another part of the vic chip, is 
sent while one or more bus lines is within the undefined voltage range. when an 
undefined voltage is latched into a register, the register enters a metastable 
state, which means that its output will flicker rapidly between zero and one 
several times before settling. this has catastrophic consequences for a dram: 
the row multiplexer will connect several different memory rows, one at a time, 
to the same sense lines. but as soon as some charge has moved from a memory cell 
to the sense line, the amplifier will pull it all the way to a one or a zero. 
if, at this point, another memory row is connected, then the charge will travel 
from the sense line into this other memory cell. in short, one memory cell gets 
refreshed with the bit value of a different memory cell. 

note that because the bus lines change from $ff to $07, only memory rows with an 
address ending in three ones are at risk of being opened simultaneously. this 
explains why corruption can only occur in memory locations ending in 7 or f. 
finally, this phenomenon hinges on the exact timing of the ras signal at the 
nanosecond level, and on many machines the critical situation simply doesn't 
occur. the timing (and thus the probability of a crash) depends on factors such 
as temperature, vic revision, parasitic capacitance and resistance of the traces 
on the motherboard, power supply ripple and interference with other parts of the 
machine such as the phase of the colour carrier with respect to the dotclock. 
the latter is assigned randomly at power-on, by the way, which could be the 
reason why a power-cycle sometimes helps. this is lft signing off. 

--------------------------------------------------------------------------------

Please could you explain why you can say this:
"the phase of the colour carrier with respect to the dotclock. The latter is 
assigned randomly at power-on". I believe the MOS-8701 works with fixed delays 
to generate the dot clock, so it should come up with always the same phase with 
respect to the color clock.

Hi!

The MOS-8701 produces a 7.88 MHz dotclock and a 4.43 MHz colour carrier from the 
same internal signal. Their ratio is exactly 16:9, which means that 16 hi-res 
pixels on the screen correspond to nine complete cycles of the colour signal.

It follows that for each hi-res pixel, the phase of the colour carrier is 
advanced by 9/16 revolutions, which is 202.5 degrees. If it starts at 0 degrees, 
then after 8 hi-res pixels it will be at 180 degrees. Hence the familiar 
red/green vertical banding that repeats after 16 pixels; red and green are 180 
degrees apart in YUV.

Now, at which pixel is the colour carrier at 0 degrees?

This depends on the timing relationship between the 8701 and the VIC chip. The 
8701 has a reset pin, but it doesn't seem to be connected in the C64. The VIC 
doesn't even have a reset pin.

So, during power-on, there will be a brief period before the 8701 is outputting 
a stable signal, and during this period the VIC state machine may or may not 
respond properly. Meanwhile, the internal clock-divide counters of the 8701 
might not even start from zero.

That is where the random assignment happens.

--------------------------------------------------------------------------------

Can any byte in ram matching the $0007 pattern be changed?
I guess what is banked out would not suffer?

Yes, and often several bytes at the same time.

Banking doesn't affect anything, I'm afraid, because the Row Select procedure 
(LSB) is carried out regardless of what the MSB will be. The corruption happens 
inside the RAM chips themselves.

================================================================================

https://csdb.dk/forums/?roomid=11&topicid=99103

2013-07-19 - New VSP discovery

First off, this is what we already knew: VSP causes the VIC chip to briefly
place a logically undefined value on the DRAM address lines during the
halfcycle following the write to d011. If the undefined value coincides with
the RAS signal, every memory cell with an xxx7 or xxxf address is at risk of
getting corrupted. The relative timing of the undefined value and RAS depends
on several factors including temperature.

We also knew that the undefined value could be delayed slightly if VSP was
triggered by setting the DEN bit instead of modifying YSCROLL. This was enough
to avoid a crash on some machines.

I wanted to investigate whether there were other ways of controlling the timing
of the undefined value. Based on a combination of educated guesswork, luck and
plenty of trial-and-error, I could observe the following: The timing depends on
the specific 3-bit value that is written to YSCROLL, as well as the 3-bit value
that was stored in YSCROLL previously.

This means that we can trigger VSP using one of 56 methods (eight different
YSCROLL values for various rasterlines, seven non-matching YSCROLL values to
switch from), each with slightly different timing.

Using the techniques from my Safe VSP demo, I created a tool that would trigger
VSP many times, check if memory got corrupted, and keep track of the number of
crashes caused by each of the 56 methods. I then looked for a pattern in these
statistics.

Intriguingly, if I arranged the 56 crash counters in a grid with the vertical
axis corresponding to the rasterline and the horizontal axis corresponding to
the exclusive-or between the rasterline and the dummy value that was stored in
d011 prior to the VSP, then the crashes would tend to occur only in a subset of
the columns. When my crash prone c64 is powered on, the VIC chip is cold, and
there are no crashes. Within a minute, crashes start to appear in column 7
(meaning that all three bits of YSCROLL were flipped). As the chip heats up,
more crashes begin to appear in columns 3, 5 and 6 (two bits flipped). After
several more minutes, crashes show up also in columns 1, 2 and finally 4 (a
single bit flipped), but by this time, there are no longer any crashes in
columns 5, 6 or 7. Finally, when the VIC chip has reached a stable working
temperature, my machine no longer crashes.

This is what it might look like four minutes after power-on:

VSP CH 1234567

LINE 0 0020537
LINE 1 0070235
LINE 2 0030322
LINE 3 0030542
LINE 4 0020443
LINE 5 0060463
LINE 6 0010526
LINE 7 0030733

Now, let me stress that I only have one VSP-crashing c64, and these results
might not carry over to other machines. I hope they do, though. I would very
much like you (yes, you!) to run VSP Lab (described below) on your crash prone
machines and report what happens.

Is this useful? Short answer: Yes, very. But it hinges on whether the behaviour
of my c64 is typical. Even without the mentioned regularity in the columns, it
would be possible to find a few safe combinations for a given machine and a
given temperature. But the regularity makes it so much more practical and also
easier to explain to all C64 users, not just coders.

Let's refer to the seven columns as "VSP channels". For a given machine at a
given temperature, some of these channels are safe, and possibly some of them
are unsafe. It takes about 5-10 minutes for the VIC chip to reach its working
temperature. If you know that e.g. VSP channel #5 is safe on your machine, and
you can somehow tell a demo or game to use that specific channel, then VSP
won't crash.

My measurement tool evolved into a program called VSP Lab, depicted above,
which you can use to find out which VSP channels are safe to use on your
machine. It triggers a lot of VSP operations and visualises the crashes in a
grid, where each column corresponds to a VSP channel. Remember that a cold and
a hot VIC behave differently, so don't trust the measurements until about ten
minutes after power-on. You can reset the grid highlights using F1 to see if
channels which were unsafe before have become safe.

Demos and games could prompt the user for a VSP channel to use, or try to
determine it automatically using the same technique that VSP Lab is based on.

From a coding point of view, all you then have to do in order to implement
crash-free VSP, is to prepare the value X that you'll write to d011 to trigger
VSP, and the value Y which is X ^ vsp_channel. Then, on the rasterline where
you want to do VSP, you just wait until the time is right and do:

        sty $d011
        stx $d011

On the VSP Lab disk image, there's a small demo effect that you can run. It
will ask you for a VSP channel to use, and if you give it a safe number, it
should not crash.

This technique is so simple and non-intrusive that it's quite feasible to patch
existing games and demos, VSP-fixing them.

Also, this discovery explains the old wisdom that if you attempt VSP more than
once per frame, the routine will be more likely to crash. Here's why: In a demo
effect, you typically perform VSP on a fixed rasterline, so the value you write
to d011 will be constant. It is reasonable to assume that the old value of
YSCROLL will also be constant. Therefore, a given VSP effect will consistently
end up in the same VSP channel. On a machine with N safe VSP channels, the
probability of survival is therefore p = N / 7. If you do VSP on two different
rasterlines, each VSP will likewise end up in a channel, but not necessarily
the same one. The probability that both end up in a safe channel is p*p. If we
assume that most crash prone machines have at least one safe channel, we have
0 < p < 1 and therefore p*p < p. Q.E.D. To verify this, I patched vice to
report the channel every time VSP was performed. Sure enough, VSP&IK+
consistently uses VSP channel 1, as does Royal Arte. Krestage 3 uses VSP
channel 2. The intro of Tequila Sunrise, which performs VSP twice per frame,
uses VSP channels 1 and 3, and so does Safe VSP.

Finally, I will attempt to explain the observed behaviour at the electronical
level. Suppose each bit of YSCROLL is continually compared to the corresponding
bit in the Y raster counter, using XOR gates. The outputs of the XOR gates are
routed to a triple-input NOR gate, the output of which is therefore high if and
only if the three bits match. A triple-input NOR gate in NMOS would consist of
a pull-up resistor and three pull-down transistors. But the output of the NOR
gate is not a perfect boolean signal, because the transistors are not ideal.
When they are closed, they act like small-valued resistors, pulling the output
towards -- but not all the way down to -- ground potential. When YSCROLL
differs from the raster position by three bits, all three transistors
contribute, and the output reaches a low voltage. When the difference is two
bits, only two transistors pull, so the output voltage is slightly higher. For
a one-bit difference, the voltage is even higher (but still a logic zero, of
course). When we trigger VSP, all transistors stop pulling the voltage down,
and because of the resistor, the output voltage will begin to rise. But the
time it needs in order to rise to a logic one depends on the voltage at which
it begins. Thus, the more bits that change in YSCROLL, the longer it takes
until the match signal is asserted.

I have a fair amount of confidence in this theory, but need more data to
confirm it. And, once again, this is only of practical use if the average crash
prone machine has safe channels, like mine does. So please check your
equipment! I'm looking forward to your reports.

--------------------------------------------------------------------------------

Time to back up my claims with more data.

I added a logging feature to VSP Lab (VSP Lab V1.1), that counts the number of crashes
on each VSP channel for one hour and plots a histogram. One minute corresponds
to five horizontal pixels, and the start of each minute is indicated with a
small dot below the chart. There is no built-in function to save the chart to
disk, but with a monitor cartridge it's just a matter of:

S"FILENAME",8,A000,AA00


Hedning gallantly lent me five machines, which I have measured several times.
The results largely confirm my theories, but the following additional facts
have surfaced:

1. The working temperature rises quickly for approximately ten minutes, then it
keeps rising slowly for at least an hour, probably more. This can be seen in
the charts where the machine was started cold: Changes in the set of safe
channels are close together towards the left end of the graph and more
drawn out towards the right.

The machine also appears to cool down rather quickly when you switch off the
power.

2. At power-on, the machine selects randomly among a handful of "behaviours".
It will stick with this behaviour across resets (neither the VIC chip, the RAM
nor the system clock generator are connected to the reset signal), but can
change if you do a power-cycle. Remember that the 1541 Ultimate reset button by
default also resets the emulated 1541, so there's no need to power-cycle a C64
unless you're going to plug/unplug hardware.

Here are the logs from two of Hedning's machines that we can call H4 (serial
number 491239) and H5 (311520). They are not displayed in chronological order.
Rather, I have identified two behaviours for each machine, and sorted the
charts according to these.

H5, behaviour 1:

Cold start:

h5l1.png
h5l3.png

Warm start:

h5l2w5.png
h5l2w2.png
h5l2w4.png

H5, behaviour 2:

Cold start:

h5l2.png

Warm start:

h5l1w1.png
h5l2w1.png
h5l3w1.png
h5l2w3.png

The last one of those is possibly a third behaviour for this machine.

H4, behaviour 1:

Cold start:

h4l1.png
h4l3.png

Warm start:

h4l3w1.png
h4l3w4.png

To quote Devia in a different thread:

"in my experience a c64 running VSP code with no problems, might fail after a
power cycle while continuing to run stable between resets."

Clearly, behaviour 1 of machine H4 (once it's warmed up) is precisely such a
stable mode, but a power-cycle might drop us into:

H4, behaviour 2:

Cold start:

(not captured)

Warm start:

h4l3w2.png
h4l3w3.png

Both machines H4 and H5 use the new VIC chip and the short motherboard.
However, Soundemon has reported VSP crashes on a 5-luma VIC in a breadbox.

Five bits are flipping as the address bus goes from $ff to $07. Three major
factors contribute to the timing of the five bit transitions with respect to
the RAS signal: The power-on behaviour, the number of set bits in the VSP
channel number, and the temperature. Apart from this, the rasterline and the
specific VSP channel will also affect the timing due to process variations, but
this is less pronounced.

The five bits don't necessarily flip at the same time. The relative timing of
the bit-flips appears to be stable for a given machine. If any of these
coincide with RAS, there is risk for a crash.

As you can see from the charts, there is a general pattern of progressing from
the 3-bit channel (7) via the 2-bit channels (3, 5, 6) to the 1-bit channels
(1, 2, 4). On channel 7, the bit-flips are late. As temperature goes up, they
get even later, and the bit-flip that coincided with RAS gets pushed out of the
critical zone. Meanwhile, if we perform VSP on channel 3, 5 or 6, the bit-flips
are a bit earlier and don't coincide with RAS. As temperature goes up, a
bit-flip gets pushed into the critical zone. Sometimes, like in the first two
charts, we see crashes disappearing from channels 1, 2 and 4 only to appear
later in channel 7. What we see is the next of the five bit transitions
approaching RAS.

Depending on the relative timing of the five bit-flips, there could be machines
which consistently crash on all channels. If you have such a machine, I'm
afraid these techniques will not help you.

Otherwise, the practical method for avoiding VSP crashes goes something like
this: Power on the machine. Run VSP Lab for ten minutes and look at the log.
Note which channels are unsafe; try to predict the next thing that will happen,
based on the ordering described above. E.g. if some 1-bit channels recently
became unsafe, then the remaining 1-bit channels might also become unsafe
within the next hours. On the other hand, if some 1-bit channels recently
became safe, then you should be more worried about channel 7 becoming unsafe.
Note down what VSP channel is safe, and select this channel when watching demos
or playing games. Don't power-cycle the machine, but use the reset button!
Every time you power-cycle, you have to do the whole procedure again, running
VSP Lab and looking at the log, although you won't have to wait the full ten
minutes if the machine is already warm.

If your machine crashes on all channels when warm, but not when cold, then you
might be able to fix it by mounting a fan over one of the ports at the back of
the machine.

If a machine survives several hours of VSP Lab without a single crash, and you
can repeat this several times with power-cycles in between, then you can be
fairly confident that the machine will never crash on VSP.

I propose the following:

1. Demo/game coders should include a VSP channel selector (if VSP is used). We
should add options in Vice to crash on a subset of the channels, to help with
debugging.

2. For home use, a machine only needs to be safe on one channel. This will
finally allow many people to watch the latest demos on real hardware.

3. Compo machines should be safe on all channels. This can be verified using
VSP Lab.

The great benefit is #2, which requires some effort on the part of the coders
(#1). A side-benefit of VSP Lab is that party organisers now have an easy way
to verify #3.

--------------------------------------------------------------------------------

also see: http://wiki.icomp.de/wiki/VSP-Fix
