This directory contains some tests related to the serial shift register of the
CIA6526(A).


cia-sp-test-oneshot-new.prg
cia-sp-test-oneshot-old.prg
---------------------------

serial shiftregister irq checks derived from the protection of arkanoid
(http://csdb.dk/release/?id=40451)

border color shows the test result:

red: no irq occured at all
yellow: number of occured interrupts is wrong
green: passed

NOTE: this test fails on hoxs64 v1.0.8.7, micro64 1.00.2013.05.11 - build 714,
      x64sc 2.4.15 r29241

cia-sp-test-continues-new.prg
cia-sp-test-continues-old.prg
-----------------------------

same as above but using continues timer

cia-icr-test-continues-new.prg
cia-icr-test-continues-old.prg
cia-icr-test-oneshot-new.prg
cia-icr-test-oneshot-old.prg
cia-icr-test2-continues.prg
cia-icr-test2-oneshot.prg
-----------------------------

related ICR ($dc0d) behaviour checks
