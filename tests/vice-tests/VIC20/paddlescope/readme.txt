To shed some light into the discussion about the Paddle jitter, here's 
Paddlescope (requires RAM in block1 / +8k)

It displays the POTY paddle signal with a 208x256 pixel screen mode. Start with 
SYS 8192, stop with STOP/RESTORE. With POKE8371,8 you can select POTX for 
display, POKE8371,9 switches back to POTY.

You see a periodic, noisy signal, which is *not* jittery because of a generally 
bad power supply of VIC-I and/or the paddles! Rather, one can see a direct 
influence by the video signal itself (especially the black signal around the 
vertical retrace), which indicates a coupling between the video amplifier and 
the supply voltage of the VIC-I chip. That in turn changes the comparator 
threshold, and this ultimately changes the paddle readout with analog paddles.

The program uses a timer NMI to read the paddle register every 512 cycles.

With PAL, a frame needs 71x312 cycles to be displayed. On screen, this gives a 
period of ~43.3 pixels. Five periods nearly fit, ~216 pixels compared to 208 of 
the display mode, which is why the "peaks" slowly roll to the right.

The program also works with NTSC, even if the display is cropped around all 
sides. Here, the same calculation gives (with 65x261 cycles for a frame) a 
period of 33.1 pixels. Six periods correspond to 199 pixels, so any signal 
correlated to the video signal will roll slowly to the left. 
