
border-<n>:
-----------
Opens right border in cycle 56 of line n and following lines.
While n=250 is early enough to let the border open, n>=251 will make the
bottom border flipflop set and so the border isn't open.
n=251 shows a bug in VICE-2.0 and earlier.

border-bm-idle:
---------------
Opens right border in the line before VIC-II changes from Hires bitmap to
idle state after last display line.
The open border seems to prevents the VIC-II from switching to
black as it usually does in idle state. Exploits a bug in VICE that is
also visible in Krestology/Crest.

border-bm-ysh(2):
-----------------
Creates idle lines within display area and opens side border. This exposes
bug in VICE showing that black idle color starts in display area while
the left open border still shows bitmap background color starting in the
open right border in the line before.

border-mcbm:
------------
Another test showing that idle data in mc bitmap mode should be displayed
the "mc way" with double sized pixels which VICE doesn't do.


vborder:
--------
Test to determine the correct cycle for vborder flipflop switch.
1. Adjust the first delay for the switch to 24 row mode with keys 'A' and 'S'
so that the upper/lower border opens. $33 should be the first value that opens it
2. Adjust the second delay for the switch back to 25 row mode with keys 'K' and 'L'
to determine the smallest delay with borders still open. That delay should be $09.

vborder2:
---------
Clears RSEL for four cycles somewhere (variable delay with 'A' and 'S') at line 247.
$00-$21: Border4 closes at line 251 (complete last line visible)
$22-$35: Border closes in line 247 (displays 4 raster lines of the last row)
$36-$63: Border closes in line 248 (displays 5 raster lines of the last row)
$64-$80: Border4 closes at line 251 (complete last line visible)

hvborder1:
----------
opens the vertical border first, then opens the sideborder. idle gfx is shown
in the entire vertical border.

hvborder2:
----------
opens the sideborder before the vertical border starts, and keeps opening it.
no idle gfx is shown in the vertical border.
