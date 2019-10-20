test1.s

this program tests the behaviour when an IRQ and NMI trigger at the same cycle,
and/or during a CLI. a timer IRQ and NMI respectively is started with varying
offsets and then the value read from a reference timer is written to screen,
which results in a characteristic pattern.

after the timing has been measured, the results are compared with a reference
dump taken from a real C64, equal values are marked green and no equal are red.

test1vic20.s - similar test program that runs on unexpanded VIC-20
