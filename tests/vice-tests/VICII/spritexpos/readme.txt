
inspired by https://www.lemon64.com/forum/viewtopic.php?p=951534

This is a little test program to confirm what the sprite x position left to
position 0 ("-1") is.

On PAL there is a gap of 8 pixels in the coordinates, so one pixel left of
position 0 is position $1f7.

On NTSC however, there is (a bit unexpected) no such gap, so left to position 0
the next position is $1ff.

The reason for this could be that NTSC was originally using 64 cycles per line,
and 64*8 is exactly 512, and when they changed the chip to produce 65 cycles
per line the resulting gap in the coordinates was inserted at a different
position.

keys:

q / w to move the sprite left and right
a     to select another sprite

TODO: open sideborder to show the behaviour in the while visible range
