This directory contains various VIA tests, ported from Andre Fachats 
"ciavarious" programs.

The idea is that Andres programs cover a bunch of things that likely matter for
the VIA too. The respective tests have been reworked to match what the original
tests want to do as close as possible. Consequently tests for cascaded timers,
and TOD clock tests, have been removed, as VIAs dont have these features.

the tests should be run on a VIC20 with (at least) 8k RAM expansion (block1)

xvic -default -memory 8k via<N>.prg

when run, each program will cycle through all sub tests once and then display
the results. red/green characters at bottom left indicate success/failure of
the sub tests. red/green border indicates success/failure of the complete test
(shows red if one of the sub tests failed). at the top of the screen you can
see the measured data, at the bottom the reference data.

--------------------------------------------------------------------------------

       (r31270)

VIA1:   works      Timer A / B
VIA2:   works      Timer A / B

VIA3:   works      Timer A / B IRQ Flags
VIA3a:  works      Timer A IRQ Flags

VIA4:   works      Timer A (toggle timer A timed/continous irq)
VIA4a:  works      Timer A (toggle timer A timed/continous irq)

VIA5:   works      Timer A / B IRQ Flags
VIA5a:  works      Timer A / B IRQ Flags

VIA9:   fail (abdefghjkl)  Timer B (toggle timer B counts PB6/Clock)

VIA10:  fail (dh)  Port B (output timer at PB7 and read back PB)
VIA11:  fail (dh)  Port B (output timer at PB7 and read back PB)
VIA12:  fail (dh)  Port B (output timer at PB7 and read back PB)
VIA13:  fail (dh)  Port B (output timer at PB7 and read back PB)

(gpz) reference data comes from the respective 1541 tests, was tweaked and
      checked against my VIC20

TODO:

- add reading and/or writing Timer A latches to all tests
- test input latching
- test serial shift register

--------------------------------------------------------------------------------
Following is a brief overview of how certain CIA features are related to the
respective VIA features (in reality it can be assumed that CIA was actually
developed by using the VIA masks and extending them - simply because that would
save a lot of time and eventually very expensive test runs).

CIA      VIA

$dc00 -> $9111  Port A Data
n/a      $911f  Port A Data (no handshake)
$dc01 -> $9110  Port B Data
$dc02 -> $9113  Port A DDR
$dc03 -> $9112  Port B DDR

$dc04 -> $9114  Timer A lo
$dc05 -> $9115  Timer A hi
n/a      $9116  Timer A Latch lo
n/a      $9117  Timer A Latch hi
$dc06 -> $9118  Timer B lo
$dc07 -> $9119  Timer B hi

$dc08 -> n/a    TOD 10th sec
$dc09 -> n/a    TOD sec
$dc0a -> n/a    TOD min
$dc0b -> n/a    TOD hour

$dc0c -> $911a  Synchronous Serial I/O Data Buffer

$dc0d -> $911d  IRQ CTRL  (w:Enable Mask / r:Acknowledge)
         $911e  IRQ flags

$dc0e ->        CTRL A (TimerA)
$dc0f ->        CTRL B (TimerB)

         $911b  Aux Control (TimerA, TimerB)
         $911c  Peripherial Control

- no cascade mode for timers
- timers run always
  - VIA1 timer B can be stopped by configuring it to count PB6 transitions. 
- only first timer can be output at port B
- no TOD clock
+ seperate register for the Timer A latch

http://www.zimmers.net/anonftp/pub/cbm/documents/chipdata/6522-VIA.txt
http://www.zimmers.net/anonftp/pub/cbm/schematics/computers/vic20/251027l2.gif

--------------------------------------------------------------------------------
