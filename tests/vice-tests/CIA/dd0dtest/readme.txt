dd0dtest.prg by Wilfred Bos

- fails on x64 and x64sc (r31051)

The test program is reading the value of $DD0D before, during and after a NMI
occurs in several ways by doing:

lda $dd0d

ldx #$00
lda $dd0d,x

ldx #$10
lda $dcfd,x

ldx #$10
lda $ddfd,x

inc $dd0d

ldx #$00
inc $dd0d,x

Some scenarios that are missing from the tests:

ldx #$10
inc $dcfd,x

ldx #$10
inc $ddfd,x

ldy #$00
lda ($fe),y             ; where $fe and $ff points to $dd0d

ldx #$00
lda ($fe,x)             ; where $fe and $ff points to $dd0d

and scenarios that are executing code on the CIA registers which can also read
the $dd0d register (e.g. executing a RTS or RTI instruction at $DD0C) are
missing.

Possible values that are in the output of the tests:

00 - no NMI occured, no Timer A or B Interrupt
01 - Timer A Interrupt
02 - Timer B Interrupt
81 - NMI occured, Timer A Interrupt
82 - NMI occured, Timer B Interrupt
FF - default init value, writing of DD0D value to output value was not performed

Tests acktst1 (Test 01) and acktst2 (Test 02) test how long it takes before the
NMI is acknowledged and a new NMI is triggered. There is a different behavior in
the old and new CIA model.

Tests 03 until 16 are performed in a loop. These tests will test if the NMI is
acknowledged at the right cycle during execution of an instruction that is
reading DD0D. The NMI occurs every 22 cycles.

Tests dd0dzero0delaytst0 (Test 17) until dd0dzero0delaytst2 (Test 19) test the
behavior of disabling Timer A while enabling Timer B via one instruction and
check the DD0D value. There seems to be difference in the old and new CIA model.

Some example of how to interpret the test result:

If e.g. the result is:

TEST 0C READ  010101010101 FAILED
     EXPECTED 0101FFFF8181

This means that for test0c the first two output values are correct, namely $01.
The code that writes the value of "sta output+2" and "sta output+3" should not
have been reached because an NMI occurs before these instructions and therefore
the values are not written, hence the value FF as the expected value.

However the value of DD0D is read as $81 just before the "sta output+2"
instruction and then written during NMI execution as the 5th value of the
output. The NMI was not acknowledge while the last read of DD0D resulted in $81
just before the execution of the code in the NMI routine. Therefore the reading
of DD0D is still $81 in the NMI routine, hence the 6th value is also $81.

TEST 0C tests the "inc $dd0d,x" for reading DD0D. Note that the execution of this
instruction is like:

Tn  Address Bus   Data Bus          R/W   Comments
T0  PC            OP CODE           1     Fetch OP CODE
T1  PC + 1        BAL               1     Fetch low order byte of Base Address
T2  PC + 2        BAH               1     Fetch high order byte of Base Address
T3  ADL: BAL + X  Data (Discarded)  1
    ADH: BAH + C
T4  ADL: BAl + X  Data              1     Fetch Data
    ADH: BAH + C
T5  ADH, ADL      Data              0
T6  ADH, ADL      Modified Data     0     Write modified Data back intro memory
T0  PC + 3        OP CODE           1     New Instruction

Since in this example the test fails, it seems like the "inc $dd0d,x" is not
acknowledging the NMI at cycle T3 but at cycle T4 and therefore the next NMI is
occuring one cycle later which is then acknowledged in the test and therefore the
NMI routine is never executed.
Cycle T3 is discarding the data but it does actually read the data which should
also acknowledge the NMI when reading DD0D.

TODO

Still these tests are not exactly written for the new CIA model. The values are
checked for the new model but it will not test the same behavior at the right
cycle. Some tests should run one cycle earlier or later in order to test the
same behavior.
