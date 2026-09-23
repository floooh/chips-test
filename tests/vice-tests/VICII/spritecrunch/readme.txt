
The normal Structure of Sprites looks like this:

00, 01, 02,
03, 04, 05,
06, 07, 08,
 :   :   :
3c, 3d, 3e

So there are always 3 Byte-Steps, but if you use Sprite-Crunching, you can
change that Structure.

Set the bits of $d017 before the start of the right sideborder and clear them
two cycles before the end of the left sideborder and SCR will show its magic!

Then the SpritePositionByte (first of the 3 Bytes in a Sprite row) has
influence on the effect.
Also notice that a Sprite only ends if the SpritePositionByte would be $3f,
otherwise the Sprite warps over and starts at $00 or $01 again.

1/ SPB of the actual SCR line
2/ Diff to the normal step
3/ SPB of the next line

00: +2 01     01: -1 05     02: impossible
03: -1 07     04: +2 05     05: +3 05
06: +4 05
09: -1
0c: ...

--------------------------------------------------------------------------------

spritecrunch.prg:

does trigger the sprite crunch condition once at the top of the sprite.

use 1/2 to move the cycle where the first write to $d017 happens
use a/s to move the cycle where the second write to $d017 happens

spritecrunch2.prg:

does the d017 writes every rasterline, and every 8th line the timing is shifted
by one cycle.

use 1/2 to move the whole timing

