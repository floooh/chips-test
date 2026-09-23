bug: https://sourceforge.net/p/vice-emu/bugs/1627/

some discussion: (starting at #28) https://csdb.dk/forums/?roomid=11&topicid=84186

--------------------------------------------------------------------------------

This program creates badlines after cycle 14 on every 4th rasterline, starting
with rasterline $30.

Since the badline causes a line crunch, rasterline $30 shows the last line of
the characters that are in memory locations $0400 ... $0427 (although the first
ones are hidden by the FLI bug).

On real C64s, the colors are mid grey (which is the color stored in $d800 ...
$d827), which one would expect since the VIC c-accesses read 12 bits at a time.


"vscroll" is updated in cycle 15 (when first cycle is counted as zero), hence
"badline" state is active in cycle 16.

"badline" state disables idle mode but display logic recognize it in cycle 17.

In actual code "dmli" and "vmli" are incremented in each visible cycle but for
"vmli" this happens only in NON idle mode. (which is right)

"dmli" should be incremented under same conditions.
