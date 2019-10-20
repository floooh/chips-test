Transactor magazine (February 1989 on page 62)., George Hug 

"[...]
It appears that many 6526 CIA chips have a hardware defect involving the
interrupt flag for Timer B. If Timer B times out at about the same time as a
read of the interrupt register, the Timer B flag may not be set at all. Under
the VIA emulation, Timer B will then underflow and count down Sffff cycles
before generating another NMI. A whole series of incoming bytes may be lost as
a result. The defect was present in five of six C128s and two of three C64s
sampled. When "good" and "bad" chips were switched, the problem followed the
"bad" chip. There appear to be no such defects with respect to the flags for
Timer A or FLAG. This glitch can cause errors during simultaneous I/O - when
Timer A generates the NMI and Timer B times out just as the service routine
reads $dd0d.
[...]
Program 3 ('ciatest64 v ) tests for the glitch in Timers A and B of CIA#2. Load
and run in 64 mode only, without the card edge connector. Only a Timer B glitch
has been found so far."
