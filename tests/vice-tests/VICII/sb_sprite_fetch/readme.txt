Test Prog(s) for Sprite Data Fetch in sideborder
------------------------------------------------

If Sprite X-Position is >=$164, the sprite data that was fetched during the 
sprite fetch cycles on the very same raster line will be shown.

This test prog was done to show how the sprite x position influences the sprite 
data shown on the screen.

sbsprf24.prg:
-------------

F1/F3 to move the selected sprite
F5/F7 to change the selected sprite

in the border >= xpos $164 the first line of the sprite (which is the one we
are looking at) will contain an animated dot going left and right, to prove
the theory below is actually working.

sbsprf24-163.prg:
sbsprf24-164.prg:
-----------------

variations of the above for automatic testing (do not change!)


Details:
--------

Usually the ypos for a sprite is one less than the rasterline it should actually 
show up, e.g. if you want to see a sprite in rasterline $33, you have to put $32 
in its ypos-reg.

Now if a sprite has xpos >= $164, the sprite display is turned on in the very 
same line. For sprite 0, this behaviour can be exploited as the sprite data is 
read in the cycles directly before. This was done e.g. to get 9 sprites in one 
rasterline.

If we put any other sprite at this position (xpos >= $164), the display will 
also start immediatly on the same rasterline (not on the following one). The 
displayed sprite data corresponds to the data that was on the vic bus during 
those fetch cycles.

As the described behaviour occurs on the same rasterline as the ypos-value of 
the sprite, there has not been a valid sprite data fetch before during the 
cycles belonging to that sprite. What happened instead during those two sprite 
fetch cycles?

1. In the first halfcycle (with the bus connected to the vic) the sprite pointer 
   was read
2. in the second halfcycle (now with bus connected to cpu) the next cpu cycle is 
   executed -> some mechanism must generate the first $ff-byte at this point
3. in the next halfcycle (now again bus connected to vic) the vic does an idle 
   access to the "ghostbyte" $3fff as the normal sprite data fetch is not active 
   yet
4. in the last halfcycle (bus belongig to cpu again) the same should happen as 
   in the 2nd halfcycle

The sprite data fetch will be done on the hardwired cycles belongig to that 
sprite even if display is not active! If we have e.g. sprite3 at xpos=$164, we 
will see the data that was fetched at cycles 1 and 2 of the very same raster-
line! But as the sprite dma was not active yet during these fetch cycles, the 
sprite data buffer for sprite no.3 is filled with the data the VIC "sees" during 
those fetch (half)cycles (e.g. vic-bus-data during 2nd halfcycle of cycle 1 for 
first buffer byte, ghostbyte for second buffer byte and vic-bus-data during 2nd 
halfcycle of cycle 2 for third buffer byte).

To change the value from the default pulled up state, perform a write (or read) 
access that generates the desired byte on the internal bus on the right cycles.

Placing a value on the internal bus is done by writing or reading a vic-ii 
register. As there is no dma going on it is possible to do this in the right 
spot. If you want to do this for both ff's go for a rmw instruction.

One use for this is just to hide the upper bogus line even though the border is 
open.

It works with arbitrary data by preloading a vic register with the first byte,
then doing sta <vicreg>,x with the second byte.

By correctly placing the read/write-cycles of this command, the first $ff-byte 
is read from "abs+x (w/o hi-byte correction)" whereas the 2nd $ff-byte is 
exactly what's written to the adress (content of the accu in this case).

see also https://csdb.dk/forums/?roomid=11&topicid=103169
