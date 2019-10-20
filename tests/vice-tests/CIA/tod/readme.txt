
In this directory we collect some tests related to the TOD clock of the CIA.

--------------------------------------------------------------------------------

For some reason the TOD clock is neither documented very well in the original
datasheet, nor was it ever investigated in detail by anyone else until now - the
following is an attempt to collect the available info and prove it piece by
piece in seperated test programs.

--------------------------------------------------------------------------------

First, this is what the datasheet says:

The TOD clock is a special purpose timer for real-time applications. TOD 
consists of a 24-hour (AM/PM) clock with 1/10th second resolution. It is 
organized into 4 registers: 10ths of seconds, Seconds, Minutes and Hours. The 
AM/PM flag is in the MSB of the Hours register for easy bit testing. Each 
register reads out in BCD format to simplify conversion for driving displays, 
etc. 

The clock requires an external 60 Hz or 50 Hz (programmable) TTL level input on 
the TOD pin for accurate time-keeping. (->stability.prg, stability-ntsc.prg)

In addition to time-keeping, a programmable ALARM is provided for generating an 
interrupt at a desired time. (->alarm.prg, alarm-cond.prg, alarm-cond2.prg)

The ALARM registers are located at the same addresses as the corresponding TOD 
registers. Access to the ALARM is governed by a Control Register bit. The ALARM 
is write-only; any read of a TOD address will read time regardless of the state 
of the ALARM access bit. (->powerup.prg)

A specific sequence of events must be followed for proper setting and reading 
of TOD. TOD is automatically stopped whenever a write to the Hours register 
occurs. The clock will not start again until after a write to the 10ths of 
seconds register. This assures TOD will always start at the desired time. 
(->write-stop.prg)

Since a carry from one stage to the next can occur at any time with respect to a 
read operation, a latching function is included to keep all Time Of Day 
information constant during a read sequence. All four TOD registers latch on a 
read of Hours and remain latched until after a read of 10ths of seconds. The TOD 
clock continues to count when the output registers are latched. (->read-latch.prg)

If only one register is to be read, there is no carry problem and the register 
can be read "on the fly," provided that any read of Hours is followed by a read 
of 10ths of seconds to disable the latching. (->read-latch.prg)

--------------------------------------------------------------------------------

Register format:

REG  NAME           D7      D6      D5      D4      D3      D2      D1      D0
8    TOD 10THS      0       0       0       0       T8      T4      T2      T1
9    TOD SEC        0       SH4     SH2     SH1     SL8     SL4     SL2     SL1
A    TOD MIN        0       MH4     MH2     MH1     ML8     ML4     ML2     ML1
B    TOD HR         AM/PM   0       0       HH1     HL8     HL4     HL2     HL1

Read:  always TOD
Write: CRB7=0 TOD       CRB7=1 ALARM

REG D (ICR) bit2 (TODIRQ)  (read)  1=alarm time reached (->alarm.prg)
                           (write) 1=enable/disable alarm irq
REG E (CRA) bit7 (TODIN)   1=50Hz clock input
                           0=60Hz clock input
REG F (CRB) bit7 (ALARM)   1=writing to TOD registers sets ALARM,
                           0=writing to TOD registers sets TOD clock.

--------------------------------------------------------------------------------

More facts about the TOD clocks that are not in the datasheet:

- the individual digits of the hour, min, sec and tsec registers are binary
  counters, and the carry over to the next digit is performed by a direct
  comparison by value. "invalid" values work and will be counted up as expected
  within the limits of the respective register (->fix-hour.prg, fix-min.prg, 
  fix-sec.prg, fix-tsec.prg)
- writing 12 pm into hour register turns to 12 am and vice versa.
  apparently cia constantly monitors writes to hour register and mechanically
  flips am/pm bit whenever hour value changes to 12, no matter whether value
  is poked in from outside or is result of carry from minutes register.
  (->hour-test.prg)
- reading the clock (hour to tenths) does not start it when it is stopped
  (->powerup.prg)
- the frequency counter is being reset to 0 when the clock was stopped and is
  restarted (->hzsync0.prg, hzsync1.prg)
- Wikipedia says: "Due to a bug in many 6526s, the alarm IRQ would not always 
  occur when the seconds component of the alarm time is exactly zero. The 
  workaround is to set the alarm's tenths value to 0.1 seconds."
  TODO: make test!

- at powerup (->powerup.prg)
  - the clock is not running (must be started by writing to 10th second register)
  - the clock "latched" status is cleared
  - the time value read from the latch is 01:00:00.0 (mostly, hour might be $11,
    minute might be $01 ...)
  - the am/pm flag is random (mostly 0)
  - the alarm is set to 00:00:00.0 by default, and because of that does not
    trigger unless the time is forced to 00:00:00.0 too

- after reset (->powerup.prg)
  - the clock is not running (must be started by writing to 10th second register)
  - the clock "latched" status is cleared
  - the time value read from the latch is 01:00:00.0
  ...(?)

Also note that the C64 kernal does not initialize or touch the TOD clock in any 
way (the BASIC RND function will read it), which means it can be tested without 
the kernal interfering.

--------------------------------------------------------------------------------

* alarm.prg:

check if ALARMTIME = TIME sets bit 2 of ICR

* alarm-cond.prg:  

checks exact conditions for when bit 2 if ICR gets set (set time to current 
alarm time)

* alarm-cond2.prg: 

checks exact conditions for when bit 2 if ICR gets set (set alarm time to 
current time)

--------------------------------------------------------------------------------

* fix-hour.prg, fix-min.prg, fix-sec.prg, fix-tsec.prg:

these test check the behaviour of the respective registers when invalid values
have been written to them.

--------------------------------------------------------------------------------

* 0alarm.prg, 1alarm.prg

tests that reproduce how the game "slurpy" uses the TOD clock.

expected: exactly one alarm per test pass

NOTE: this test fails in VICE (r28032, fixed in r28654)

--------------------------------------------------------------------------------

* 4tod.prg, 4todcia.prg:

this one repeatedly starts tod at 0:00:00.0 with alarm-nmi enabled
when either the alarm is triggerd or tod ran for 1 second it will print out
whether an alarm-nmi occured or not.

expected: NO alarm in 2nd run

NOTE: this test fails in VICE (r28032, fixed in r28654)

--------------------------------------------------------------------------------

* 5tod.prg:

this one repeatedly starts tod at 0:00:00.0 with alarm-nmi enabled
when either the alarm is triggerd or tod ran for 1 second it will print out
whether an alarm-nmi occured or not.

this version will set the clock to the power up state of 1:00:00.0 after every
test that didn't trigger an alarm, to see if the weird behaviour of the 2nd pass
can be repeated.

expected: first run alarm, second NO alarm, third alarm, etc

NOTE: this test fails in VICE (r28032, fixed in r28654)

--------------------------------------------------------------------------------

* 6tod.prg:

this one repeatedly starts tod at 0:00:00.0 with alarm-nmi enabled
when either the alarm is triggerd or tod ran for 1 second it will print out
whether an alarm-nmi occured or not.

this version will always reset the clock to the power up state of 1:00:00.0

expected: always alarm

--------------------------------------------------------------------------------

* hour-test.prg:  

check AM/PM flag of the hour register (see hour-test.txt for details)

--------------------------------------------------------------------------------
following tests check the reset condition(s) of the frequency counter:

* hzsync0.prg:

find out if the 50/60Hz counter is running freely or somehow synchronized with 
writes to TOD-time:
in the latter case .1secs should change roughly 5 frames after last rewrite
in the former case .1secs should change about 2 frames after last rewrite

(slight variations of +/-1 frames can be expected as neither frame rate nor ToD 
input are exactly 50 Hz.)

expected: all "synced" (mostly 5s, some 6s)

* hzsync1.prg:

expected: all "free running" (mostly 1s, some 2s)

* hzsync2.prg:

writing to seconds/minutes while clock is running 

expected: no reset (5s)

* hzsync3.prg:

omitting writing to seconds/minutes while clock is stopped

expected: counter still resets (mostly 1s, some 2s)
 
* hzsync4.prg:

changing the input frequency (50/60Hz) while clock is running
 
expected: no reset (mostly 3s, some 4s)

* hzsync5.prg:

stopping the clock for a few frames, then restaring it (to see if the counter is 
reset by write to hour reg, but still running while clock is stopped)

expected: counter is either not running while clock is stopped or reset when 
          clock is restarted (which pretty much has the same outcome). (5s)

--------------------------------------------------------------------------------

* powerup.prg

checks the state of the TOD clock at power-on

note: i have seen $01 (mostly) but also $11 (rare) in the hour register when
      it is $11, often also the am/pm flag is 1, too. (gpz) 

--------------------------------------------------------------------------------

* read-latch.prg

checks that reading the hour register latches the current time, further reads
will neither "unlatch" nor "relatch" until tenths are read.

checks that min-tsec can be read "on the fly"

--------------------------------------------------------------------------------

* stability.prg, stability-ntsc.prg

measures time between 10th seconds alarms, and shows the result. values shown
are the minimum and maximum amount of cycles between 10th second ticks, the
difference, average and ideal value.

in reality these values may vary surprisingly much. infact, anything between
50.2Hz and 49.8HZ is ok for normal operation of the power grid. values between
50.02Hz and 49.08Hz will not get compensated at all.

- on my c64c the min/max difference is up to ~100 cycles depending on time of
  day or whatnot. also the average time/frequency varies (gpz)

--------------------------------------------------------------------------------

* write-stop.prg

checks if the clock stops (only) when writing to the hour register, and restarts
(only) when writing to tsec register.

also checks that writing to the tod registers does neither start nor stop the
clock when alarm is selected by setting CRB bit 7. ("mapping the 64" claims it 
does)

--------------------------------------------------------------------------------

* hammerfist0.prg

This program sets the CIA 1's TOD to a specific value (9:30:12.3) then resets
it, and, if the TOD alarm was set to the same specific value, prints a D in the
top left corner, otherwise an A.

On the real C64, it always prints D.

How this was discovered: The game "Hammerfist" periodically does this check,
most probably as a piracy prevention method. The loader sets the TOD alarm.
If the check fails during the game, a spurious PLA is executed, corrupting the
stack and leading to a crash or other undesired behaviour.

However, it was noticed by some users of a forum that a pirate copy of
Hammerfist, that crashes often in VICE, works well on a real C64.

(fixed in r28654)

--------------------------------------------------------------------------------

* hammerfist1.prg

A new version of the test program. It resets the TOD alarm to 0:00:00.0,
then it sets the TOD clock to 9:12:30.3. Then, it checks for bit 2 of
$DC0D (meaning: TOD alarm and TOD time are identical) and print X if it
is 0, something else if it is 1. Then it sets the TOD time to all
zeroes, byte by byte (first $DC08, last $DC0B) and performs the test of
bit 2 of $DC0D each time.

Run it under VICE: it prints XXXXX in the top left corner
Run it under a real C64: it prints XXXXD in the top left corner

(fixed in r28654)

--------------------------------------------------------------------------------

* frogger.prg

Timer countdown used in the game "frogger". It relies on the state of registers
and latches after poweron/reset.

--------------------------------------------------------------------------------

TODO: 

- investigate "slurpy", try to create a testcase from it (see bug #514)
- test behaviour of the latch in more detail
 - test writing time while time is latched
 - test when exactly the value is copied into the latch (synced to freq. count?)
- what happens if the frequency counter is switched from 60 to 50 hz when it has
  reached the value 6 ? will it count further to 7 ?
- investigate/test what exactly is tied to the frequency counter, what to the
  system clock, and how both interacts/how certain border cases work.
- check if/when IRQs are generated when the clock is stopped
