
posted by "Freak" here:

https://www.forum64.de/index.php?thread/92992-bug-im-vic-ii-data-sequencer-im-u64/

--------------------------------------------------------------------------------
(following was translated and adapted from the original post)


"I have been dealing with a bug in the U64 in the demo "Shards Of Fancy" from 
Lft during the weekend.

You can see pixel flickering (above the S of GREETS) on the top right, which can 
only be seen on the U64 and not on real C64 machines or emulators.

In the demo these pixels are in line 51 (where this is an idle line because the 
screen in the demo was dragged down one line and only starts at line 52).

I looked at the code of the demo in detail and cobbled together my own little 
program with the knowledge I had.

The upper right corner is important here. With real machines and also in 
emulators this place is always black, with the U64 it is filled with bright, 
different pixels.

The behavior of the U64 seems to be completely in order: Line 51 is in idle 
state, because $d011 contains the value $3c at the beginning of line 51 
(i.e. the VIC-II wants to display the bitmap from line 52, where the small 
square is on the left side...). At said right position in line 51 the data 
sequencer is forced from idle state to display state by means of $3b -> ($d011). 
And that immediately, as it says in the VIC-II document by Christian Bauer 
(chapter 3.7.1).

The light blue bars are stretched sprites, but they are not important here. I'm 
sure that writing to $d011 and creating a "bad line" condition at that time must 
be the trigger.
