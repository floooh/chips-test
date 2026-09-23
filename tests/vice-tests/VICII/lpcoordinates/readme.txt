
On a negative edge on the LP input, the current position of the raster beam
is latched in the registers LPX ($d013) and LPY ($d014). LPX contains the
upper 8 bits (of 9) of the X position and LPY the lower 8 bits (likewise of
9) of the Y position. So the horizontal resolution of the light pen is
limited to 2 pixels.

--------------------------------------------------------------------------------

The test program shows the LPX and LPY latches in the upper left on the screen,
and also the other joystick port bits, and POTX/POTY, to allow checking for the
"trigger" button as well.

Pressing CBM cycles through some different types of compensating the values,
as described below.

"ideal" - no compensation
"lightpen" - 28 pixels (x)
"magnum light phaser" - 72 pixels (x)

To determine the approximate delay/offset of a device, point it at the marker
at the top right of the screen. With an ideal device the resulting values in
the LP latches are $7c (x) and $5a (y). Substract those from the actually
latched values and you get the offset. 

Point at the marker and hold "run/stop" to calculate the offset values and 
switch to custom calibrated mode

Pressing "arrow up" turns the border on/off. Switch it to black (off) to
determine the lowest/highest values.

--------------------------------------------------------------------------------

Theoretical exact values:

- X values match sprite X coordinate / 2
- Y values match current rasterline, ie sprite Y coordinate - 1 (the sprite 
  starts one rasterline later than its Y coordinate)

lowest X value  : $0c (=12)     *2 -> $18 (=24)
highest X value : $ab (=171)    *2 -> $157 (=343)

lowest Y value  : $32 (=50)
highest Y value : $fa (=250)

Now, in reality, the Y position is quite accurate (depending mostly on the
optical properties of the devices, but can be 100% accurate also in practise).
The X position will be delayed a bit, depending on the device (depending on the
type of photo transistor/diode etc).

(gpz) With my Lightpen (a simple homemade one with no buttons):

-> x position delayed ~28 pixels

lowest X value  : $1a (=26)     *2 -> $34 (=52)
highest X value : $c0 (=192)    *2 -> $180 (=384)

lowest Y value  : $34 (=52)
highest Y value : $fa (=250)

(gpz) With my Magnum Lightphaser:

-> x position delayed ~72 pixels (with quite some jitter, ~8 pixels)

lowest X value  : $30 (=48)     *2 -> $60 (=96)
highest X value : $d0 (=208)    *2 -> $1a0 (=416)

lowest Y value  : $35 (=53)
highest Y value : $fa (=250)
