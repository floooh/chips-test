$9000 split testcase written by tokra

PAL-VIC20 with at least +8K required.

First the program fills the screen with the characters from 0 to 255 twice 
(just 506 visible, screen ratio still 22 x 23 chars)

Then it synchronizes with the raster bream and changes $9000 to 0 at cycle 13 
of rasterline 114. At cycle 26 of rasterline 132 it resets the value of $9000 
to 12 and then loops with constant synchronization to the raster beam, so what 
you see on the screen is stable.

Then after pressing SPACE the value written to $9000 at cycle 13 of rasterline 
114 is increased by one (so now 1 is written) and will increase further after 
each press of SPACE. This way you can see what changing the value of $9000 to 
different values will do to the screen. Furthermore the value which $9000 is 
changed to will be poked to $1000 (the first byte of the video-RAM), so you can 
see by that at which point of the experiment exactly you are the moment.

reference pictures (tokra):

At value 24 the line below the split looks a little garbled. At value 25 
suddenly the same char-line starts with a lower char (Y instead of Z). At value 
26 it even skips to start with W! Position 27 starts with V, and then at 
position 28 it suddenly jumps to the pound-sign just to go back to S at 
position 29. It continues to skip a character or two back which each next step, 
until suddenly at position 36 all bets are off, as it seems to jump FORWARD 
about 140 chars, just to go back at position 37. The next jump like that is at 
position 52 (36+16), until finally with position 68 (36+16+16) finally the real 
machine catches up with VICE and from then on they look the same.

does NOT work in VICE (r33582):

"From value 26 onwards a blank line appear instead of the top-line of the next 
 char-line. And from position 68 onwards suddenly the last char-line is being 
 repeated. This stay the same until value 127."
