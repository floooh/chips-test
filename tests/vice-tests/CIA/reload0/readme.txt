the "core" of the test looks like this (each column of values in comments
refer to one case that fails, ie the first few of cia1ta)

   ; write values "init"
   ldx i4
   stx $dc04    ; i4   $00 $00 $00 $00  clear timer lo
   ldx #$10
   stx $dc0e    ;                       stop timer
   bit $dc0d    ;                       ack irq
   sta $dc0e    ; ie   $11 $11 $11 $11  start timer

   ; write values "before"
   lda b4
   sta $dc04    ; b4   $14 $14 $14 $14  set timer lo
   sty $dc0e    ; be   $00 $01 $08 $09  switch timer mode

   ; get values "after"
   lda $dc04    ; a4   $10 $0e $10 $0e  (should be $0f/$0d/$0f/$0d)
   ldx $dc0d    ; ad   $81 $81 $81 $81
   ldy $dc0e    ; ae   $00 $01 $08 $09

and the values printed are this:

failed test #08
init    00    11 <- i4    ie
before  14    00 <- b4    be
after   10 81 00 <- a4 ad ae
right   0f 81 00 <- expected (a4 ad ae)
         ^  ^  ^
         |  |  \_ dc0e (ctrl1)
         |  \____ dc0d (irq mask)
         \_______ dc04 (timerA lo)

the problem only exists if:

- i4 is $00
- ie is $11 (not if it is $18 or $19, ie NOT in oneshot mode)

reload0a.prg:
-------------

top block shows "a4", middle "ad" and bottom "ae" values of the above test.
the test cycles through all "be" configurations horizontally, and "b4" value
decreses each row.

reload0b.prg:
-------------

top block shows the "a4" value for each of the 8 "be" configurations
($00,$01,$08,$09,$10,$11,$18,$19) per line.
the "ie" value is always $11. "i4" is always $00 and "b4" value deccreases from
left to right (starting with 20)

the bottom block shows the difference between the reference data and the result.
(1 / "A" meaning the value read is one more than the value that was expected)