

0123456789ABCDEF        via1
**..*******..*..
          ^  ^
0123456789ABCDEF        via2
.*....*****..*..
          ^  ^

*) marked are not written to by reset + loading the test program
^) marked are definately the default reset values

--------------------------------------------------------------------------------
also see: drive/defaults/defaults.prg

Rockwell Datasheet says:
"A low reset input clears all internal registers to logic 0 (except T1 and T2
latches and counters and the Shift Register). This places all peripheral
interface lines in the input state, disables the timers, shift register, etc.
and disables interrupting from the chip.

                  pwr reset
0 pb data             $00 (guessed)
1 pa data             $00 (guessed)
2 pb ddr=in           $00 (guessed)
3 pa ddr=in           $00
4 timer1 lo         ?     (value not reset?)
5 timer1 hi         ?     (value not reset?)
6 timer1 lo latch   ?     (value not reset?)
7 timer1 hi latch   ?     (value not reset?)
8 timer2 lo         ?     (value not reset?)
9 timer2 hi         ?     (value not reset?)
a shift reg       $00     (value not reset?)
b Aux. control        $00
c Per. control        §00 (guessed)
d IRQ flags           $00
e IRQ enable          $00 (guessed)
f pa data no hs       $00 (guessed)

