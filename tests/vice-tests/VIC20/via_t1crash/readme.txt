This tests for a bug introduced by r30700 - when all timers are set to 0, the
emulator would hang (in viacore) with unresponsive GUI. (fixed in r30719)

via1crash.prg:
    sets all VIA1 timers to 0, should return "all ok"

via2crash.prg:
    sets all VIA2 timers to 0, should return "all ok".
    NOTE: will not return to basic

basic programs have a load address suitable for 8k expansion, load with ,8 on
unexpanded VIC
