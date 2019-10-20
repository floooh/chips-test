
the following describes the various keyboard related test programs - scroll down
to the bottom for the conclusions :)

================================================================================

ciaports.prg
------------

This test checks the behaviour of the CIA1 ports when both are programmed as
output, connected via the keyboard matrix, and then read back.

the interesting cases are:

* Port A outputs high, Port B outputs (active) low.

- In this case port B always "wins" and drives port A low,
  meaning both read back as 0.

* Port A outputs (active) low, Port B outputs high.

This case is a bit special and has some interesting properties:

- Usually (probably unexpectedly) port B will NOT be driven low by port A,
  meaning port A reads back 0 and port B reads back 1.

- port B will be driven low (and then read back 0) only if the resistance of the
  physical connection created over the keyboard matrix is low enough to allow
  the required current. this is (again) usually not the case when pressing single
  keys, instead -depending on the keyboard- pressing two or more keys of the same
  column is required. a special case is the shift-lock key, which will also work
  and which you can seperate from the normal left shift key in this configuration.

ghosting.prg
------------

this test scans the keyboard in a few common and some not so common ways and
shows the results.

NOTE: the following observations are the results of testing on one c64 only, and
      need further investigation.

top:

* port A -> port B (inactive port A bits high)
  - the common case. certain 3-key combinations produce ghostkeys (sdf, awd ...)

* port A -> port B (inactive port A bits input)
  - behaves like the common case

* port B -> port A (inactive port B bits high)
  - certain 3-key combinations that would produce ghostkeys in the common case
    will NOT produce ghostkeys here.
  - even some 4-key (and more!) combinations will not produce ghostkeys (sdfg,
    zvbn+space ...)

* port B -> port A (inactive port B bits input)
  - behaves like the common case

bottom:

* port B -> port B (one bit output, others input. port A is input)
  - if two keys in the same row are pressed, that combination shows up here
    (each bit indicating an active column)

* Port A outputs (active) low, Port B outputs high. scanning with port A 
  (inactive bits are high and input), reading port B.
  - shift-lock will exclusively show up here, NO other keys
  - pressing two or more keys in the same row with shift-lock makes shift-lock
    disappear
    -> One bit in Port A drives at most two connected Port B bits low

* port A -> port A (one bit output, others input. port B is input)
  - if two keys in the same column are pressed, that combination shows up here
    (each bit indicating an active row)

* Port A outputs (active) low, Port B outputs high. scanning with port B, 
  (inactive bits are low), reading port A.
  - this always reads 0s

================================================================================

Conclusions:
------------

The following is an attempt to describe the exact behaviour in the various
possible configurations, based on the observations above

output <-> input (the "legal" cases)

* Port A outputs (active) low, Port B is input
  * reading Port B
    - the common case, one port A bit will drive any number (?) of connected
      port b bits.

* Port B outputs (active) low, Port A is input
  * reading Port A
    - in this configuration one active port B bit which is connected to port A
      via more than one key will not show up as 0 in port A, if this line is also
      connected to an inactive high bit of port B. it *will* read 0 if all other
      connected bits of port B are inputs.

* port A -> port A (one bit output, other input)
  * reading Port A
    - like the common case

* port B -> port B (one bit output, other input)
  * reading Port B
    - like the common case

output <-> output (the "illegal" cases)

* Port A outputs high, Port B outputs (active) low.
  * reading Port A
    - port B always "wins" and drives port A low.
  * reading Port B
    - always low, Port B will never be driven high.

* Port A outputs (active) low, Port B outputs high.
  * reading Port A
    - always low, Port A will never be driven high.
  * reading Port B
    - Usually (probably unexpectedly) port B will NOT be driven low by port A.
    - connected via shift-lock, one port A bit is enough to drive port B low
    - One bit in Port A drives at most three connected Port B bits low.
