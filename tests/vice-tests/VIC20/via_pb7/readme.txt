
this test checks behaviour of PB

- Set ACR(reg B) bits 7 and 6 to certain value.
- Read value of PB.
- Start timer with write 0x01 to timer-counter-H (reg 5).
- Read value of PB again.
- Change ACR/reg B to new test pattern.
- Read PB again.
- Wait until timer overflows
- Read value of PB again

top line of each "packet" is with PB DATA=0, bottom with DATA=1 ...
rows top down are ACR (start) = $00,$40,$80,$c0 and
colums from left to right are ACR (end) = $00,$40,$80,$c0

the correct output looks like this:

ACR           ACR(end)=
(start)  $00  $40  $80  $c0
=
$00     @@@@ @@@@ @@b@ @@b@  data=0
        cccc cccc ccca ccca  data=1

$40     @@@@ @@@@ @@b@ @@b@  data=0
        cccc cccc ccca ccca  data=1

$80     b@@@ b@@@ b@@b b@@b  data=0
        cacc cacc caac caac  data=1

$c0     b@@@ b@@@ b@@b b@@b  data=0
        cacc cacc caac caac  data=1

(gpz) reference data checked on my vic20

fails on VICE r31270


xvic -memory 8k main-exp.prg
xvic main.prg
