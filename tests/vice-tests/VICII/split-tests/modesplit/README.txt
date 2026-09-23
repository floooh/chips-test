Readme for modesplit
--------------------

The reference "screenshots" were crafted to match the supposed expected results.

CAUTION: since this is a tedious and error prone process, there might still be
errors left. Corrections welcome!

modesplit.prg.png (PAL 6569)
    (matched against my Breadbox, gpz 15/12/2025)
modesplit.prg-8565.png (PAL 8565)
    (matched against my C64C, gpz 15/12/2025)
modesplit.prg-8565early.png (PAL 8565)
    (created from the above, gpz 15/12/2025 - happens to match hoxs64 exactly)
modesplit.prg-ntsc.png (NTSC 6567)
    (matched against screenshot(s), gpz 15/12/2025)
modesplit.prg-8562.png (NTSC 8562)
    (matched against screenshot(s), gpz 15/12/2025)

-R04: changed code to use videobank 2 ($8000-$bfff) and fill entire videoram
 with known pattern(s)- this way no garbage will show up anywhere and automatic
 testing becomes much more predictable :)

-----------------------------------------------------------------------

dumps/modesplit-6567r8.jpg  (purple pixel in camera)
--------------------------
Dumped on Machine 1 (Jason Compton):

C64: NTSC Breadbox
Ser# P01815269
Rev C

CPU: MOS/6510 CBM/0684
CIA1: MOS/6526/0884 (CIA-U1)
CIA2: MOS/6526/0784 (CIA-U2)
VIC: MOS/6567R8/2684
SID: MOS/6581/2484


dumps/modesplit-6569r3.jpg
--------------------------
Dumped on Machine 1 (tlr):

C64: PAL Breadbox
Ser# U.K.B 1521345

CPU:  MOS/6510 CBM/3184
CIA1: MOS/6526/3884
CIA2: MOS/6526 R4/3283
VICII: 6569R3 (guess)
SID: <unknown>


dumps/modesplit-6572.jpg
------------------------
Dumped on Machine 1 (Thierry):

Drean Commodore 64

CPU: MOS/6510 CBM/4683
CIA1: MOS/6526/4585  (U1)
CIA2: MOS/6526A-1/0786 (U2)
VICII: MOS/6572R0/4785 S (ceramic)


dumps/modesplit-8562r4.jpg  (purple pixel in camera)
--------------------------
Dumped on Machine 1 (Jason Compton):

C64C: NTSC (newer-style keys)
Ser# CA1362224
Rev 3

CPU: MOS/8500 R3/0786
CIA1: MOS/6526A/0788 216A (CIA-U1)
CIA2: MOS/6526A/0788 216A (CIA-U2)
VICII: MOS/8562 R4/4987 24
SID: <unknown>


dumps/modesplit-8564.jpg
------------------------
Dumped on Machine 1 (Jason Compton):

C128D NTSC
Ser# 045208

CPU: MOS/8502R0/2688 20
CIA1: MOS/6526A/3588 216A (CIA-U1)
CIA2: <unknown> (under PSU) (CIA-U4)
VICII: <unknown>
SID: MOS/8580R5/3488 25


dumps/modesplit-8564r5.jpg  (purple pixel in camera)
--------------------------
Running on C128 NTSC.

Dumped on Machine 1 (Jason Compton):

C128: NTSC flat
Ser# CA2001686
Mfg date 9/86

CPU: MOS/8502R0/1286
CIA1: MOS/6526/3086 S (CIA-U1)
CIA2: MOS/6526/3086 S (CIA-U4)
VICII: MOS/8564R5CA  V 6/4585
SID: <unknown>


dumps/modesplit-8565.jpg
------------------------
Dumped on Machine 1 (Rubi):

CPU: CSG/8500/3390 24
CIA1: CSG/6526A/3590 216A
CIA2: CSG/6526A/3590 216A
VICII: 8565R2 (guess)
SID: <unknown>
