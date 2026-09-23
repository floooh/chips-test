vicii_timing.prg - combined videomode/color split test by peter wendrich
--------------------------------------------------------------------------------

some notes:

- the differently compiled tests fill the "bitmap" (charset) data with different
  patterns. the original tests uses 0x5a, while the alternative one uses 0xa5 or
  0xff respectively.

  the only difference(s) caused by this are visible in the EMM and E+B tests.


