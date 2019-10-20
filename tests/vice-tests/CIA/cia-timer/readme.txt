
cia-timer
=========

- eight tests using timers of CIA1/2 and check ICR near the timer underrun
- for upper tests interrupt is disabled, only the IRC is read and displayed
- lower tests enable the timer interrupt and IRC is checked even inside the 
  interrupt handler

  CIA1TA     CIA1TB
   ICR=0      ICR=0

  CIA1TA     CIA1TB
   ICR=1      ICR=1

  CIA2TA     CIA2TB
   ICR=0      ICR=0

  CIA2TA     CIA2TB
   ICR=1      ICR=1


Results for real boxes
======================

C64C Rubi (2x6526A):
C64C gpz (CIA1:6526A, CIA2:6526):
(this is equivalent to dump-newcia.bin)

CIA-TIMER R02 / RUBI

.......@abcdefgh    .......@abcdefgh
@@@@@@@@aaaaaaa@    @@@@@@@@bbbbbbb@
aaaaaaaaa@@@@@@@    bbbbbbbbb@@@@@@@

.......@abcdefgh    .......@abcdefgh
@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@
@@@@AAAAA@@@@@@@    @@@@BBBBB@@@@@@@
AAAA@@@@ AAAA       BBBB@@@@ BBBB   
kkkkgggg bbbb       kkkkgggg bbbb

.......@abcdefgh    .......@abcdefgh
@@@@@@@@aaaaaaa@    @@@@@@@@bbbbbbb@
aaaaaaaaa@@@@@@@    bbbbbbbbb@@@@@@@

.......@abcdefgh    .......@abcdefgh
@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@
@@@@AAAAA@@@@@@@    @@@@BBBBB@@@@@@@
AAAA@@@@@AAAA       BBBB@@@@@BBBB   
kkkkggggbbbbb       kkkkggggbbbbb


C64 tlr (2x6526):
(this is equivalent to dump-oldcia.bin)

.......@abcdefgh    .......@abcdefgh  <- '.' is inverted graphics
@@@@@@@@aaaaaaa@    @@@@@@@@@bbbbbb@
aaaaaaaaa@@@@@@@    @bbbbbbbb@@@@@@@

.......@abcdefgh    .......@abcdefgh  <- '.' is inverted graphics
@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@
@@@AAAAAa@@@@@@@    @@@BBBBBb@@@@@@@
AAA@@@@  AAA        .BB@@@@  .BB      <- '.' is inverted '@'   
kkkgggg  bbb        kkkgggg  bbb


.......@abcdefgh    .......@abcdefgh  <- '.' is inverted graphics
@@@@@@@@aaaaaaa@    @@@@@@@@@bbbbbb@
aaaaaaaaa@@@@@@@    @bbbbbbbb@@@@@@@

.......@abcdefgh    .......@abcdefgh  <- '.' is inverted graphics
@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@
@@@AAAAAa@@@@@@@    @@@BBBBBb@@@@@@@
AAA@@@@@ AAA        .BB@@@@@ .BB      <- '.' is inverted '@'   
kkkggggb bbb        kkkggggb bbb
