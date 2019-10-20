Readme for cia-int
------------------

-----------------------------------------------------------------------

Running on Breadbox ca 1983-84:

Machine 1 (tlr):

C64: PAL Breadbox
Ser# U.K.B 1521345

CPU: MOS/6510 CBM/3184
CIA1: MOS/6526/3884
CIA2: MOS/6526 R4/3283
VICII: 6569R3 (guess)
SID: <unknown>

Machine 2 (Jason Compton):

C64: NTSC Breadbox
Ser# P01815269
Rev C

CPU: MOS/6510 CBM/0684
CIA1: MOS/6526/0884 (CIA-U1)
CIA2: MOS/6526/0784 (CIA-U2)
VIC: MOS/6567R8/2684
SID: MOS/6581/2484

Machine 3 (Jason Compton):

C128: NTSC flat
Ser# CA2001686
Mfg date 9/86

CPU: MOS/8502R0/1286
CIA1: MOS/6526/3086 S (CIA-U1)
CIA2: MOS/6526/3086 S (CIA-U4)
VICII: MOS/8564R5CA  V 6/4585
SID: <unknown>

Machine 4 (The_Woz):

Drean C64, mainboard: 1984 version (8 RAM chips, 8701 clock generator)

CPU: MOS/8500 R3/5185
CIA1: MOS/6526/4485
CIA2: MOS/6526/4485
VICII: 6572 R0 (ceramic)

Machine 5 (Thierry):

Drean Commodore 64

CPU: MOS/6510 CBM/4683
CIA1: MOS/6526/4585  (U1)
CIA2: MOS/6526A-1/0786 (U2)
VICII: MOS/6572R0/4785 S (ceramic)

Machine 6 (nojoopa):

C64: PAL Breadbox

CPU: MOS/6510/1385
CIA1: MOS/6526/1685
CIA2: MOS/6526/1685

  CIA-INT (IRQ) R04 / TLR

  DC0C: A9 XX 60
  --BBBb------------------
  AACC--IIIIKK------------

  DC0C: A5 XX 60
  --BBBb------------------
  AADDD-JJJJJLL-----------

  DC0B: 0D A9 XX 60
  --BBBBb-----------------
  AAEEEE-KKKKKMM----------

  DC0B: 19 FF XX 60
  --BBBBb-----------------
  AA-----LLLLLLNN---------

  DC0C: AC XX A9 09 28 60
  --BBBb------------------
  AA----HHHMMMMMOO--------

  [Note: this matches reference data for "PAL/old CIA" (cia-int-irq.prg)]

  CIA-INT (NMI) R04 / TLR

  DD0C: A9 XX 60
  --BBBb------------------
  AACCI-IIIIKKMMQQQQSS----

  DD0C: A5 XX 60
  --BBBb------------------
  AADDD-JJJJJLLNNRRRRTT---

  DD0B: 0D A9 XX 60
  --BBBBb-----------------
  AAEEEE-KKKKKMMOOSSSSUU--

  DD0B: 19 FF XX 60
  --BBBBb-----------------
  AAFFFF-LLLLLLNNPPTTTTVV-

  DD0C: AC XX A9 09 28 60
  --BBBb------------------
  AAEEE-HHHMMMMMOOQQUUUUWW

  [Note: this matches reference data for "PAL/old CIA" (cia-int-nmi.prg)]

Machine 1 (tlr):

CPU: MOS/6510  CBM/2484
CIA1: MOS/6526/4682
CIA2: MOS/6526R-4/2484

Machine 2 (nojoopa):

CPU: MOS/6510/1385
CIA1: MOS/6526/1685
CIA2: MOS/6526/1685


  CIA-INT R03 / TLR

  DC0C: A9 XX 60
  --BBBb------------------
  AACC--IIIIKK------------

  DC0C: A5 XX 60
  --BBBb------------------
  AADDD-JJJJJLL-----------

  DC0B: 0D A9 XX 60
  --BBBBb-----------------
  AAEEEE-KKKKKMM----------

  DC0B: 19 FF XX 60
  --BBBBb-----------------
  AA-----LLLLLLNN---------

  DC0C: AC XX A9 09 28 60
  --BBBb------------------
  AA----HHHMMMMMOO--------

  [Note: this matches reference data for "PAL/old CIA" (cia-int-irq.prg)]
 

Running on Drean C64C:

Machine 1 (hedning):

Drean C64C (SC-136712), rev a-motherboard from 1982 (assy: 326298).
CPU: MOS/6510/0683
SID: MOS/6581/5182 
CIA1: <unknown>
CIA2: <unknown>


  CIA-INT (IRQ) R04 / TLR

  DC0C: A9 XX 60
  --BBBb------------------
  AACC--IIIIKK------------

  DC0C: A5 XX 60
  --BBBb------------------
  AADDD-JJJJJLL-----------

  DC0B: 0D A9 XX 60
  --BBBBb-----------------
  AAEEEE-KKKKKMM----------

  DC0B: 19 FF XX 60
  --BBBBb-----------------
  AA-----LLLLLLNN---------

  DC0C: AC XX A9 09 28 60
  --BBBb------------------
  AA----HHHMMMMMOO--------

  [Note: this matches reference data for "PAL/old CIA" (cia-int-irq.prg)]

-----------------------------------------------------------------------

Running on a C64C:

Machine 1 (Rubi):

CPU: CSG/8500/3390 24
CIA1: CSG/6526A/3590 216A
CIA2: CSG/6526A/3590 216A

Machine 2 (Jason Compton):

C64C: NTSC (newer-style keys)
Ser# CA1362224
Rev 3

CPU: MOS/8500 R3/0786
CIA1: MOS/6526A/0788 216A (CIA-U1)
CIA2: MOS/6526A/0788 216A (CIA-U2)
VICII: MOS/8562 R4/4987 24
SID: <unknown>

Machine 3 (Jason Compton):

C128D NTSC
Ser# 045208

CPU: MOS/8502R0/2688 20
CIA1: MOS/6526A/3588 216A (CIA-U1)
CIA2: <unknown> (under PSU) (CIA-U4)
VICII: <unknown>
SID: MOS/8580R5/3488 25


  CIA-INT R03 / TLR

  DC0C: A9 XX 60
  ---BBB------------------
  AAACC-IIIIIKK-----------

  DC0C: A5 XX 60
  ---BBB------------------
  AAADDDJJJJJJLL----------

  DC0B: 0D A9 XX 60
  ---BBBB-----------------
  AAAEEEEKKKKKKMM---------

  DC0B: 19 FF XX 60
  ---BBBB-----------------
  AAA----LLLLLLLNN--------

  DC0C: AC XX A9 09 28 60
  ---BBB------------------
  AAA---HHHMMMMMMOO-------

  [Note: this matches reference data for "PAL/new CIA" (cia-int-irq-new.prg)]

  CIA-INT (NMI) R04 / TLR

  DD0C: A9 XX 60
  ---BBB------------------
  AAACCIIIIIIKKMMQQQQS----

  DD0C: A5 XX 60
  ---BBB------------------
  AAADDDJJJJJJLLNNRRRRT---

  DD0B: 0D A9 XX 60
  ---BBBB-----------------
  AAAEEEEKKKKKKMMOOSSSSU--

  DD0B: 19 FF XX 60
  ---BBBB-----------------
  AAAFFFFLLLLLLLNNPPTTTTV-

  DD0C: AC XX A9 09 28 60
  ---BBB------------------
  AAAEEEHHHMMMMMMOOQQUUUUW

  [Note: this matches reference data for "PAL/new CIA" (cia-int-nmi-new.prg)]

-----------------------------------------------------------------------
It works like this:
- start timer
- jsr $dc0b or $dc0c depending on test.
- show contents of $dc0d at the time of ack and if an IRQ occured.

The test is done for timer values of 0-23 (timer B used).
The top line of each test shows the contents of $dc0d.
(B=$82, b=$02, -=$00)
The bottom line shows the number of cycles since starting the test until an IRQ occured. (- means no IRQ)

Test 1:  Ack in second cycle of 2 cycle imm instr.
DC0C A9 XX    LDA #$xx
DC0E 60       RTS

Test 2:  Ack in second cycle of 3 cycle zp instr.
DC0C A5 XX    LDA $xx
DC0E 60       RTS

Test 3:  Ack in third cycle of 4 cycle abs instr.
DC0B 0D A9 XX ORA $xxA9  <-- memory prefilled to do ORA #$xx (mostly)
DC0E 60       RTS

Test 4:  Ack in third cycle of 5 cycle abs instr. (force page crossing)
DC0B 19 FF XX ORA $xxFF,Y <-- memory prefilled to do ORA #$xx (mostly)
DC0E 60       RTS

Test 5:  Ack in second cycle of 4 cycle instr.
DC0C AC XX A9 LDY $A9xx  <-- memory prefilled to do LDY #$xx
DC0F 08       PHP  <-- this can also be ORA #$28 if the timer hasn't expired yet.
DC10 28       PLP
DC11 60       RTS

-----------------------------------------------------------------------
eof

