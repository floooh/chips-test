dma delay tests
---------------

for the basic theory refer to vic article "3.14.6. DMA DELAY"

test1 and test2 implements the first method, which is initializing YSCROLL to non
zero before line $30 so there is no badline condition in line $30. then mid-line
set YSCROLL to zero and back to non zero some cycles later. test1 will do the
later using inc/dec and test2 uses stx/sty.

test3 implements the second method, which is initializing YSCROLL to zero and
clearing DEN before line $30, so there would be a badline condition, but the
badline will not start unless DEN was set for at least one cycle in line $30 -
which is then done after the variable delay.

each test shows a hex pattern in the first two screen rows ($0400-$044f). you
should see "89ABCDEF0123456789ABCDEF0123456789ABCDEF" in the top row and
"0123456789ABCDEF" in the bottom right corner of the screen when the test starts
up. some color bars are shown behind to demonstrate that the timing is stable.

the two white numbers show the following:

- the delay before first initialization of $d011 (if it is initialized too late,
the effect will not work. in test3 at a certain point the screen will switch off)
this delay can be changed by pressing "1" and "2"

- the delay before forcing the badline to start, adding one cycle to the default
scrolls the screen by one character (this is the delay you would usually alter 
to make actual use of dma delay). this can be changed by pressing "A" and "S"

--------------------------------------------------------------------------------

the skeleton for these tests was shamelessly copied from this example at
codebase64: 

http://codebase64.org/doku.php?id=base:horizontal_screen_positioning_hsp

--------------------------------------------------------------------------------

test4 refers to the bug reported in https://sourceforge.net/p/vice-emu/bugs/218/
and is based on the posted program.

There's a DMA delay about 8 pixels below the sprites. The behaviour on real C64 
(and C128) is that leftmost and rightmost 3 characters are the same; which chars 
and colors are used depends on individual VIC, sometimes they even flicker. 

Neither x64 nor x64sc gets this right.

test4-c64c-m7.jpg shows output from C64C with 8565R2

test4-c64-m2.jpg shows output from C64 with 6569 R5 (matches C64RMK2 with 8565R2,
matches C64RMK1 with 6569R5)

test4-c64c-m1.jpg shows output from another C64C with 8565R2. Notable observation
is that the $<8 chars start flickering after a while (different chars)

--------------------------------------------------------------------------------

Technical background:

The VIC has an internal buffer for 40 characters and their colors so it needs to 
fetch them from memory only for the first line of each character row and can 
retrieve the previously read characters (or colors for bitmap mode) from this 
buffer for the other 7 lines. Then there's a "pointer" addressing the buffer 
entry to read or write. It's called VMLI in the well-known VIC article that 
circulates around the internet in many places and languages, calling it a "6-bit 
counter with reset input".

In reality it's not a counter but a 40-bit shift register (no proof yet but 
explains the effects best and die shots also look like a shift register being 
there). Whenever the VIC starts displaying character data it shifts in a 1; the 
shift register is shifted by one position in every cycle (not just between the 
sideborders). And there's no reset entry.

Normally, after the last character has been displayed there's no more 1 bit in 
the register, thus when characters in the next line start the shift register is 
all zeroes when the 1 gets shifted in. Furthermore. in idle mode this register 
is all zeroes, causing reads to also return all zeroes and thus causing idle 
patterns to be always black.

DMA delay delays the start of the characters in a row and thus the position 
where the 1 gets shifted in. If that happens during the last 16 visible chars 
(for PAL timing) there's still a 1 left in the shift register when another one 
gets shifted in for the next row. If the next row isn't a bad line, the VIC thus 
fetches 2 cells at once when reading character data and "mixes" their states. 
When both contain different bits the VIC sometimes uses the first one, the 
second one, ANDs them or ORs them or flickers between these possibilities 
(behavior on C64 is unpredictable, highly dependent on the individual VIC; C128 
however uses AND most of the time). After reading the VIC writes the values it 
just read back into the buffer, this also explains why you also see the same 
strange characters at the right border (since then the second 1 bit already has 
gone there) and the further character row's lines.

Some special cases to mention:
 * If you cause the line following the DMA delay to be a bad line you won't see 
   anything strange. While near the left border the VIC overwrites both the 
   intended buffers and the buffers near the right border still addressed by the 
   1 bit left by the DMA delay it later on overwrites those erroneously written 
   bytes.

 * On C128, when you start DMA delay near the right border but cancel it right 
   away (either by using 2 MHz mode or the test bit), the remainder of that line 
   will be displayed as if it wasn't a bad line and the next line will be idle 
   again. Even more interesting, the 1 still inside the shift register makes the 
   VIC colorize the leftmost fetched idle bytes with character/color buffer 
   contents.
