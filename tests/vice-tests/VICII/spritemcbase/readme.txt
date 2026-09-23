Behaviour: observed glitches visible for one frame when sprites are first enabled

Assumption: On 6569 machines MCBASE is not $00 from start, but $3f

Now, this wouldn't matter if MCBASE was reset on the start of sprite display 
(see: vic article "3.8.1. Memory access and display") but if MCBASE is reset 
_after_ the end of sprite display it makes sense. This shouldn't make any 
observable difference other than on the initial anomaly.

An MCBASE of $3f means 3 full laps (IIRC) through the data before reaching the 
end condition, and that the reset is after the displaying means that it would 
only happen once, which is consistent with what we are seeing.

Sometimes (very seldom) glitches of different length appeared which would 
indicate a different initial MCBASE. The initial value is probably just an 
artifact of the NMOS implementation.

On the 8565 this is not seen/was corrected, most likely by adding a reset to 
MCBASE at start up.

Summary:
- MCBASE cleared _after_ sprite display end.
- 6569: MCBASE=$3f after power up (sometimes different).
- 8565: MCBASE=$00 after power up 


spritemcbase.prg:
-----------------

dumps VIC registers at startup, installs a "reset proof" program for testing.

press 1-8 to enable sprites
