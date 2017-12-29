C64 Emulator Test Suite - Public Domain, no Copyright

The purpose of the C64 Emulator Test Suite is
- to help C64 emulator authors improve the compatibility
- ensure that updated emulators have no new bugs in old code parts

The suite are a few hundred C64 programs which check the details of the C64 they are running on. The suite runs automated and stops only if it has detected an error. That the suite doesn't stop on my C64-I/PAL proves that the suite has no bugs. That the same suite doesn't stop on an emulator proves that this particular emulator is compatible to my C64 regarding every tested detail. Of course, the emulator may still have bugs in parts which are not tested by the suite. There may also be a difference between your C64 and my C64.

While the Test Suite is running, the Datasette should be disconnected. Needs about 80 min to complete.

The source code has been developed with MACRO(SS)ASS+ by Wolfram Roemhild. The file TEMPLATE.ASM provides a starting point for adding new tests to the suite.


///////////////////////////////////////////////////////////////////////////////
Program _START - some 6510 basic commands, just as an insurance


///////////////////////////////////////////////////////////////////////////////
Programs LDAb to SBCb(EB) - 6510 command function

addressing modes
-----------------------------------
n   none (implied and accu)
b   byte (immediate)
w   word (absolute for JMP and JSR)
z   zero page
zx  zero page,x
zy  zero page,y
a   absolute
ax  absolute,x
ay  absolute,y
i   indirect (JMP)
ix  (indirect,x)
iy  (indirect),y
r   relative

Display:
before  data  accu xreg yreg  flags  sp
after   data  accu xreg yreg  flags  sp
right   data  accu xreg yreg  flags  sp

Either press STOP or any other key to continue.

All 256 opcodes are tested except HLTn (02 12 22 32 42 52 62 72 92 B2 D2 F2).

Indexed addressing modes count the index registers from 00 to FF.

JMPi (xxFF) is tested.

Single operand commands: 256 data combinations from 00 to FF multiplied by 256 flag combinations.

Two operand commands: 256 data combinations 00/00, 00/11, ... FF/EE, FF/FF multiplied by 256 flag combinations.

ANEb, LASay, SHAay, SHAiy, SHXay, SHYax and SHSay are executed only in the y border. These commands cause unpredictable results when a DMA comes between the command byte and the operand.

SHAay, SHAiy, SHXay, SHYax and SHSay are tested on a data address xxFF only. When the hibyte of the indexed address needs adjustment, these commands will write to different locations, depending on the data written.


///////////////////////////////////////////////////////////////////////////////
Programs TRAP* - 6510 IO traps, page boundaries and wrap arounds

 #  code  data  zd  zptr   aspect tested
-----------------------------------------------------------------------------
 1  2800  29C0  F7  F7/F8  basic functionality
 2  2FFE  29C0  F7  F7/F8  4k boundary within 3 byte commands
 3  2FFF  29C0  F7  F7/F8  4k boundary within 2 and 3 byte commands
 4  D000  29C0  F7  F7/F8  IO traps for code fetch
 5  CFFE  29C0  F7  F7/F8  RAM/IO boundary within 3 byte commands
 6  CFFF  29C0  F7  F7/F8  RAM/IO boundary within 2 and 3 byte commands
 7  2800  D0C0  F7  F7/F8  IO traps for 16 bit data access
 8  2800  D000  F7  F7/F8  IO trap adjustment in ax, ay and iy addressing
 9  2800  29C0  02  F7/F8  wrap around in zx and zy addressing
10  2800  29C0  00  F7/F8  IO traps for 8 bit data access
11  2800  29C0  F7  02/03  wrap around in ix addressing
12  2800  29C0  F7  FF/00  wrap around and IO trap for pointer accesses
13  2800  0002  F7  F7/F8  64k wrap around in ax, ay and iy addressing
14  2800  0000  F7  F7/F8  64k wrap around plus IO trap
15  CFFF  D0C6  00  FF/00  1-14 all together as a stress test
16  FFFE  ----  --  --/--  64k boundary within 3 byte commands
17  FFFF  ----  --  --/--  64k boundary within 2 and 3 byte commands

In the programs TRAP16 and TRAP17, the locations of data, zerodata and zeroptr depend on the addressing mode. The CPU port at 00/01 is not able to hold all 256 bit combinations.

The datasette may not be connected while TRAP16 and TRAP17 are running!

Display:
after   data  accu xreg yreg  flags
right   data  accu xreg yreg  flags

If all displayed values match, some other aspect is wrong, e.g. the stack pointer or data on stack.

All 256 commands are tested except HLTn. Registers before:
data   1B (00 01 10 11)
accu   C6 (11 00 01 10)
xreg   B1 (10 11 00 01)
yreg   6C (01 10 11 00)
flags  00
sptr   not initialized, typically F8

The code length is 6 bytes maximum (SHSay).

When the lowbyte of the data address is less than C0, SHAay, SHAiy, SHXay, SHYax and SHSay aren't tested. Those commands don't handle the address adjustment correctly.

Relative jumps are tested in 4 combinations: offset 01 no jump, offset 01 jump, offset 80 no jump, offset 80 jump. For the offset 80, a RTS is written to the location at code - 7E.


///////////////////////////////////////////////////////////////////////////////
Program BRANCHWRAP - Forward branches from FFxx to 00xx

Backward branches from 00xx to FFxx were already tested in TRAP16 and TRAP17.


///////////////////////////////////////////////////////////////////////////////
Program MMUFETCH - 6510 code fetching while memory configuration changes

An example is the code sequence LDA #$37 : STA 1 : BRK in RAM at Axxx. Because STA 1 maps the Basic ROM, the BRK will never get executed.

addr  sequence
---------------------
A4DF  RAM-Basic-RAM
B828  RAM-Basic-RAM
EA77  RAM-Kernal-RAM
FD25  RAM-Kernal-RAM
D400  RAM-Charset-RAM
D000  RAM-IO-RAM

The sequence IO-Charset-IO is not tested because I didn't find some appropriate code bytes in the Charset ROM at D000-D3FF. The SID registers at D4xx are write-only.


///////////////////////////////////////////////////////////////////////////////
Program MMU - 6510 port at 00/01 bits 0-2

Display:
0/1=0-7 repeated 6 times: values stored in 0 and 1
after  0 1  A000 E000 D000 IO
right  0 1  A000 E000 D000 IO

address  value   meaning
----------------------------------------
A000     94      read Basic, write RAM
A000     01      read/write RAM
E000     86      read Kernal, write RAM
E000     01      read/write RAM
D000/IO  3D/02   read Charset, write RAM
D000/IO  01/02   read/write RAM
D000/IO  00/03   read/write IO


///////////////////////////////////////////////////////////////////////////////
Program CPUPORT - 6510 port at 00/01 bits 0-7

Display:
0/1=00/FF repeated 6 times: values stored in 0 and 1
after  00 01 
right  00 01 

The datasette may not be connected while CPUPORT is running!

If both values match, the port behaves instable. On my C64, this will only happen when a datasette is connected.


///////////////////////////////////////////////////////////////////////////////
Program CPUTIMING - 6510 timing whole commands

Display:
xx command byte
clocks  #measured
right   #2

#1  #2  command or addressing mode
--------------------------------------
2   2   n
2   2   b
3   3   Rz/Wz
5   5   Mz
4   8   Rzx/Rzy/Wzx/Wzy
6   10  Mzx/Mzy
4   4   Ra/Wa
6   6   Ma
4   8   Rax/Ray (same page)
5   9   Rax/Ray (different page)
5   9   Wax/Way
7   11  Max/May
6   8   Rix/Wix
8   10  Mix/Miy
5   7   Riy (same page)
6   8   Riy (different page)
6   8   Wiy
8   10  Miy
2   18  r+00 same page not taken
3   19  r+00 same page taken
3   19  r+7F same page taken
4   20  r+01 different page taken
4   20  r+7F different page taken
3   19  r-03 same page taken
3   19  r-80 same page taken
4   20  r-03 different page taken
4   20  r-80 different page taken
7   7   BRKn
3   3   PHAn/PHPn
4   4   PLAn/PLPn
3   3   JMPw
5   5   JMPi
6   6   JSRw
6   6   RTSn
6   6   RTIn

#1 = command execution time without overhead
#2 = displayed value including overhead for measurement
R/W/M = Read/Write/Modify


///////////////////////////////////////////////////////////////////////////////
Programs IRQ and NMI - CPU interrupts within commands

Tested are all commands except HLTn. For a command of n cycles, a loop with the interrupt occurring before cycle 1..n is performed. Rax/Ray/Riy addressing is tested with both the same page and a different page. Branches are tested not taken, taken to the same page, and taken to a different page.

Display:
stack  <address>  <flags on stack& $14>  <flags in handler & $14>
right  <address>  <flags on stack& $14>  <flags in handler & $14>

When an interrupt occurs 2 or more cycles before the current command ends, it is executed immediately after the command. Otherwise, the CPU executes the next command first before it calls the interrupt handler.

The only exception to this rule are taken branches to the same page which last 3 cycles. Here, the interrupt must have occurred before clock 1 of the branch command; the normal rule says before clock 2. Branches to a different page or branches not taken are behaving normal.

The 6510 will set the IFlag on NMI, too. 6502 untested.

When an IRQ occurs while SEIn is executing, the IFlag on the stack will be set.

When an NMI occurs before clock 4 of a BRKn command, the BRK is finished as a NMI. In this case, BFlag on the stack will not be cleared.


///////////////////////////////////////////////////////////////////////////////
Programs CIA1TB123 and CIA2TB123 - CIA timer B 1-3 cycles after writing CRB

The cycles 1-3 after STA DD0F cannot be measured with LDA DD06 because it takes 3 cycles to decode the LDAa. Executing the STA DD0F at DD03 lets the CPU read DD06 within one cycle.

#1 #2  DD06 sequence 1/2/3 (4)
---------------------------------
00 01  keep   keep   count  count
00 10  keep   load   keep   keep
00 11  keep   load   keep   count
01 11  count  load   keep   count
01 10  count  load   keep   keep
01 00  count  count  keep   keep

#1, #2 = values written to DD0F


///////////////////////////////////////////////////////////////////////////////
Programs CIA1PB6 to CIA2PB7 - CIA timer output to PB6 and PB7

Checks 128 combinations of CRA/B in One Shot mode:
  old CRx bit 0 Start
      CRx bit 1 PBxOut
      CRx bit 2 PBxToggle
  new CRx bit 0 Start
      CRx bit 1 PBxOut
      CRx bit 2 PBxToggle
      CRx bit 4 Force Load

The resulting PB6/7 bit is:
  0 if new PBxToggle is 0
  1 if new PBxToggle is 1
  - (undetermined) if PBxOut is 0

Old values do not influence the result. Start and Force Load don't either.

Next, the programs test if PBx is toggled to 0 on the first underflow and that neither writing CRx except bit 0 nor Timer Hi/Lo will set it back to 1. The only source which is able to reset the toggle line is a rising edge on the Start bit 0 of CRx.

Another test verifies that the toggle line is independent from PBxOut and PBxToggle. Changing these two bits will have no effect on switching the toggle flip flop when the timer underflows.

The last test checks for the correct timing in Pulse and Toggle Mode.


///////////////////////////////////////////////////////////////////////////////
Program CIA1TAB - TA, TB, PB67 and ICR in cascaded mode

Both latches are set to 2. TA counts system clocks, TB counts TA underflows (cascaded). PB6 is high for one cycle when TA underflows, PB7 is toggled when TB underflows. IMR is $02.

TA  01 02 02 01 02 02 01 02 02 01 02 02
TB  02 02 02 01 01 01 00 00 02 02 02 02
PB  80 C0 80 80 C0 80 80 C0 00 00 40 00
ICR 00 01 01 01 01 01 01 01 03 83 83 83

If one of the registers doesn't match this table, the program displays the wrong values with red color.


///////////////////////////////////////////////////////////////////////////////
Program LOADTH - Load timer high byte

Writing to the high byte of the latch may load the counter only when it is not running.

writing    counter  load
------------------------
high byte  stopped  yes
high byte  running  no
low byte   stopped  no
low byte   running  no


///////////////////////////////////////////////////////////////////////////////
Program CNTO2 - Switches between CNT and o2 input

When the timer input is switched from o2 to CNT or from CNT back to o2, there must be a two clock delay until the switch is recognized.


///////////////////////////////////////////////////////////////////////////////
Program ICR01 - Reads ICR around an underflow

Reads ICR when an underflow occurs and checks if the NMI is executed.

time  ICR  NMI
--------------
t-1   00   yes
t     01   no
t+1   81   yes


///////////////////////////////////////////////////////////////////////////////
Program IMR - Interrupt mask register

When a condition in the ICR is true, setting the corresponding bit in the IMR must also set the interrupt. Clearing the bit in the IMR may not clear the interrupt. Only reading the ICR may clear the interrupt.


///////////////////////////////////////////////////////////////////////////////
Program FLIPOS - Flip one shot

Sets and clears the one shot bit when an underflow occurs at t. Set must take effect at t-1, clear at t-2.

time  set    clear
------------------
t-2   stop   count
t-1   stop   stop
t     count  stop


///////////////////////////////////////////////////////////////////////////////
Program ONESHOT - Checks for start bit cleared

Reads CRA in one shot mode with an underflow at t.

time  CRA
---------
t-1   $09
t     $08


///////////////////////////////////////////////////////////////////////////////
Program CNTDEF - CNT default

CNT must be high by default. This is tested with timer B cascaded mode CRB = $61.






*******************************************
**  U N D E R   C O N S T R U C T I O N  **
*******************************************



///////////////////////////////////////////////////////////////////////////////
Programs CIA1TA to CIA2TB - CIA timers in sysclock mode

PC64Win 2.15 bug:
  before 05/02/01/00
  after  xx
  cr     11/19
  after  xx
timer low doesn't match
