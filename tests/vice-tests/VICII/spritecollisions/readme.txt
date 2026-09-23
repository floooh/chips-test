sprite-gfx-collision-cycle.prg:
CCBBBBB    (C64, C64C)
CBBBBBB    (DTV)

sprite-sprite-collision-cycle.prg:
CC@@@@@    (C64, C64C)
C@@@@@@    (DTV)

sprite-sprite.prg: (C64, C64C)
---------@-
---------@-

sprite-sprite.prg: (DTV)
@@--@--@-@-
---------@-

TODO:
- automatically generate DTV versions as well
- verify and fix NTSC versions

--------------------------------------------------------------------------------

sprite-sprite-hi-hi.prg
sprite-sprite-hi-mc.prg
sprite-sprite-mc-hi.prg
sprite-sprite-mc-mc.prg

test the basic functionality of sprite vs sprite collisions. all non transparent
sprite pixels can collide with each other.

sprite-gfx-hi-hi.prg
sprite-gfx-hi-mc.prg
sprite-gfx-mc-hi.prg
sprite-gfx-mc-mc.prg

test the basic functionality of sprite vs gfx collisions. all non transparent
sprite pixels can collide with non background gfx pixels (in multicolor mode
this means "11" and "10" pixels).

the tests set up two sprites or one sprite and a character with a certain bit-
pattern, then the first sprite is moved horizontally by one pixel each frame and
the content of the collision register then checked and displayed - which creates
a distinctive pattern.

all tests use the same bit patterns for the sprite and gfx, for the sprite that
moves horizontally it looks like this: 00 01 00 10 00 11 00. for the stationary
sprite or character it looks like this:

00
01
10
11

this ensures all possible bit- and pixel combinations can be tested.

In the resulting pattern on screen, the top and bottom row and the leftmost and
rightmost column should never show a collision as the represent the positions
when the first sprite and the second sprite or gfx do not overlap.


