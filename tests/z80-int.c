//------------------------------------------------------------------------------
//  z80x-int.c
//
//  Test Z80 interrupt handling and timing.
//------------------------------------------------------------------------------
#include "utest.h"
#define CHIPS_IMPL
#include "chips/z80.h"

#define T(b) ASSERT_TRUE(b)

static z80_t cpu;
static uint64_t pins;
static uint8_t mem[(1<<16)];

static void tick(void) {
    pins = z80_tick(&cpu, pins);
    if (pins & Z80_MREQ) {
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if ((pins & (Z80_M1|Z80_IORQ)) == (Z80_M1|Z80_IORQ)) {
        // put 0xE0 on data bus (in IM2 mode this is the low byte of the
        // interrupt vector, and in IM1 mode it is ignored)
        Z80_SET_DATA(pins, 0xE0);
    }
}

// special tick function for IM0, puts the RST 38h opcode on the data
// bus when requested by the interrupt handling
static void im0_tick(void) {
    pins = z80_tick(&cpu, pins);
    if (pins & Z80_MREQ) {
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if ((pins & (Z80_M1|Z80_IORQ)) == (Z80_M1|Z80_IORQ)) {
        // put RST 38h opcode on data bus
        Z80_SET_DATA(pins, 0xFF);
    }
}

static bool pins_none(void) {
    return ((pins & Z80_CTRL_PIN_MASK) == 0);
}

static bool pins_m1(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_M1|Z80_MREQ|Z80_RD);
}

static bool pins_rfsh(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RFSH);
}

static bool pins_mread(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RD);
}

static bool pins_mwrite(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_WR);
}

static bool pins_m1iorq(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_M1|Z80_IORQ);
}

static void skip(int num_ticks) {
    for (int i = 0; i < num_ticks; i++) {
        tick();
    }
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    copy(start_addr, bytes, num_bytes);
}

// test general NMI timing
UTEST(z80, NMI) {
    uint8_t prog[] = {
        0xFB,               //       EI
        0x21, 0x11, 0x11,   // loop: LD HL, 1111h
        0x11, 0x22, 0x22,   //       LD DE, 2222h
        0xC3, 0x01, 0x00,   //       JP loop
    };
    uint8_t isr[] = {
        0x3E, 0x33,         //       LD A,33h
        0xED, 0x45,         //       RETN
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x0100;

    // EI
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);

    // LD HL,1111h
    tick(); T(pins_m1()); T(cpu.iff1); T(cpu.iff2);
    tick(); T(pins_none());
    tick(); T(pins_rfsh()); T(cpu.int_bits == 0);
    tick(); T(pins_none());
    tick(); T(pins_none()); pins |= Z80_NMI;
    tick(); T(pins_mread()); pins &= ~Z80_NMI;
    tick(); T(pins_none()); T(cpu.int_bits == Z80_NMI);
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());

    // the NMI should kick in here, starting with a regular refresh cycle
    tick(); T(pins_m1()); T(cpu.pc == 4);
    tick(); T(pins_none()); T(cpu.pc == 4); T(!cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // extra tick
    tick(); T(pins_none());
    // mwrite
    tick(); T(pins_none()); T(cpu.pc == 4);
    tick(); T(pins_mwrite()); T(cpu.sp == 0x00FF); T(mem[0x00FF] == 0);
    tick(); T(pins_none());
    // mwrite
    tick(); T(pins_none());
    tick(); T(pins_mwrite()); T(cpu.sp == 0x00FE); T(mem[0x00FE] == 4);
    tick(); T(pins_none());

    // first overlapped tick of interrupt service routine
    tick(); T(pins_m1()); T(cpu.pc == 0x67);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());

    // RETN
    // ED prefix
    tick(); T(pins_m1()); T(cpu.a == 0x33);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread());  T(cpu.sp == 0x00FF);
    tick(); T(pins_none()); T(cpu.wzl == 0x04); T(0 == (pins & Z80_RETI));
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread());  T(cpu.sp == 0x0100);
    tick(); T(pins_none()); T(!cpu.iff1); T(cpu.wzh == 0x00); T(cpu.pc == 0x0004);

    // continue at LD DE,2222h
    tick(); T(pins_m1()); T(cpu.iff1);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(cpu.e == 0x22);
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(cpu.d == 0x22);
}

// test whether a 'last minute' NMI is detected
UTEST(z80, NMI_before_after) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0x00, 0x00, 0x00,   // l0:  NOPS
        0x18, 0xFB,         //      JR loop
    };
    uint8_t isr[] = {
        0x3E, 0x33,         // LD A,33h
        0xED, 0x45,         // RETN
    };

    // first run, NMI pin set before last tcycle should be detected
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x0100;
    // EI
    skip(4);
    // NOP
    tick(); T(pins_m1()); T(cpu.iff1);
    tick();
    tick(); T(pins_rfsh());
    pins |= Z80_NMI;
    tick();
    pins &= ~Z80_NMI;
    // NOP
    tick(); T(pins_m1());
    tick(); T(!cpu.iff1); // OK, interrupt was detected

    // same thing one tick later, interrupt delayed to next opportunity
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x1000;
    // EI
    skip(4);
    // NOP
    tick(); T(pins_m1()); T(cpu.iff1);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // NOP
    pins |= Z80_NMI;
    tick(); T(pins_m1());
    pins &= ~Z80_NMI;
    tick(); T(pins_none()); T(cpu.iff1); // IFF1 true here means the interrupt didn't trigger
    tick(); T(pins_rfsh());
    tick(); T(pins_none());

    // interrupt should trigger here instead
    tick(); T(pins_m1());
    tick(); T(!cpu.iff1); // OK, interrupt was detected
}

// test that a raised NMI doesn't retrigger
UTEST(z80, NMI_no_retrigger) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0x00, 0x00, 0x00,   // l0:  NOPS
        0x18, 0xFB,         //      JR loop
    };
    uint8_t isr[] = {
        0x3E, 0x33,         // LD A,33h
        0xED, 0x45,         // RETN
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x1000;

    // EI
    skip(4);
    // NOP
    tick(); T(pins_m1()); T(cpu.iff1);
    tick();
    tick(); T(pins_rfsh());
    pins |= Z80_NMI;    // NOTE: NMI pin stays active
    tick();
    // NOP
    tick(); T(pins_m1());
    tick(); T(!cpu.iff1); // OK, interrupt was detected

    // run until end of interrupt service routine
    while (!cpu.iff1) {
        tick();
    }
    // now run a few hundred ticks, NMI should not trigger again
    for (int i = 0; i < 300; i++) {
        tick(); T(cpu.iff1);
    }
}

// test whether NMI triggers during EI sequences (it should)
UTEST(z80, NMI_during_EI) {
    uint8_t prog[] = {
        0xFB, 0xFB, 0xFB, 0xFB,     // EI...
    };
    uint8_t isr[] = {
        0x3E, 0x33,         // LD A,33h
        0xED, 0x45,         // RETN
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x0100;

    // EI
    skip(4);
    // EI
    tick(); T(pins_m1());
    tick(); T(pins_none());
    pins |= Z80_NMI;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // next EI, start of NMI handling
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(!cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // NMI: extra tick
    tick(); T(pins_none());
    // NMI: push PC
    tick(); T(pins_none()); T(!cpu.iff1);
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());

    // first overlapped tick of interrupt service routine
    tick(); T(pins_m1()); T(cpu.pc == 0x67); T(!cpu.iff1);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());

    // RETN
    // ED prefix
    tick(); T(pins_m1()); T(cpu.a == 0x33);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread()); T(cpu.sp == 0x00FF);
    tick(); T(pins_none()); T(0 == (pins & Z80_RETI));
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread()); T(cpu.sp == 0x0100);
    tick(); T(pins_none()); T(!cpu.iff1);

    // continue after NMI
    tick(); T(pins_m1()); T(cpu.iff1);
}

// test that NMIs don't trigger after prefixes
UTEST(z80, NMI_prefix) {
    uint8_t isr[] = {
        0x3E, 0x33,     // LD A,33h
        0xED, 0x45,     // RETN
    };

    //=== DD prefix
    uint8_t dd_prog[] = {
        0xFB,               //      EI
        0xDD, 0x46, 0x01,   // l0:  LD B,(IX+1)
        0x00,               //      NOP
        0x18, 0xFA,         //      JR l0
    };
    init(0x0000, dd_prog, sizeof(dd_prog));
    copy(0x0066, isr, sizeof(isr));

    // EI
    skip(4);

    // LD B,(IX+1)
    // trigger NMI during prefix
    tick(); T(pins_m1());
    pins |= Z80_NMI;
    tick(); T(pins_none()); T(cpu.iff1);
    pins &= ~Z80_NMI;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode, NMI should not have triggered
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // run to end of LD B,(IX+1)
    skip(11); T(cpu.iff1);

    // NOP, NMI should trigger now
    tick(); T(pins_m1());
    tick(); T(pins_none());  T(!cpu.iff1);

    //=== ED prefix
    uint8_t ed_prog[] = {
        0xFB,               //      EI
        0xED, 0xA0,         // l0:  LDI
        0x00,               //      NOP
        0x18, 0xFB,         //      JR l0
    };
    init(0x0000, ed_prog, sizeof(ed_prog));
    copy(0x0066, isr, sizeof(isr));

    // EI
    skip(4);

    // LDI, trigger NMI during ED prefix
    tick(); T(pins_m1());
    pins |= Z80_NMI;
    tick(); T(pins_none()); T(cpu.iff1);
    pins &= ~Z80_NMI;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode, NMI should not have triggered
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // run to end of LDI
    skip(8); T(cpu.iff1);

    // NOP, NMI should trigger now
    tick(); T(pins_m1());
    tick(); T(pins_none());  T(!cpu.iff1);

    //=== CB prefix
    uint8_t cb_prog[] = {
        0xFB,           //      EI
        0xCB, 0x17,     // l0:  RL A
        0x00,           //      NOP
        0x18, 0xFB,     //      JR l0
    };
    init(0x0000, cb_prog, sizeof(cb_prog));
    copy(0x0066, isr, sizeof(isr));

    // EI
    skip(4);

    // RL A, trigger NMI during CB prefix
    tick(); T(pins_m1());
    pins |= Z80_NMI;
    tick(); T(pins_none()); T(cpu.iff1);
    pins &= ~Z80_NMI;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode, NMI should not have triggered
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());

    // NOP, NMI should trigger now
    tick(); T(pins_m1());
    tick(); T(pins_none());  T(!cpu.iff1);

    //=== DD+CB prefix
    uint8_t ddcb_prog[] = {
        0xFB,                       //      EI
        0xDD, 0xCB, 0x01, 0x16,     // l0:  RL (IX+1)
        0x00,                       //      NOP
        0x18, 0xF9,                 //      JR l0
    };
    init(0x0000, ddcb_prog, sizeof(ddcb_prog));
    copy(0x0066, isr, sizeof(isr));

    // EI
    skip(4);

    // RL (IX+1), trigger NMI during DD+CB prefix
    tick(); T(pins_m1());
    pins |= Z80_NMI;
    tick(); T(pins_none()); T(cpu.iff1);
    pins &= ~Z80_NMI;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // CB prefix, NMI should not trigger
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // run to end of RL (IX+1)
    skip(15); T(cpu.iff1);

    // NOP, NMI should trigger now
    tick(); T(pins_m1());
    tick(); T(pins_none()); T(!cpu.iff1);
}

// test IM0 interrupt behaviour and timing
UTEST(z80, INT_IM0) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0xED, 0x46,         //      IM 0
        0x31, 0x22, 0x11,   //      LD SP, 1122h
        0x00, 0x00, 0x00,   // l0:  NOPS
        0x18, 0xFB,         //      JR l0
    };
    uint8_t isr[] = {
        0x3E, 0x33,         //      LD A,33h
        0xED, 0x4D,         //      RETI
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0038, isr, sizeof(isr));
    cpu.sp = 0x0100;

    // EI + IM 0 + LD SP,1122h
    skip(22);
    // NOP
    im0_tick(); T(pins_m1());
    im0_tick(); T(pins_none());
    pins |= Z80_INT;
    im0_tick(); T(pins_rfsh());
    im0_tick(); T(pins_none());
    // interrupt handling starts here
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);
    im0_tick(); T(pins_m1iorq()); T(Z80_GET_DATA(pins) == 0xFF);
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_rfsh());
    im0_tick(); T(pins_none());
    // RST execution should start here
    im0_tick(); T(pins_none());
    // mwrite (push PC)
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_mwrite());
    im0_tick(); T(pins_none());
    // mwrite (push PC)
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_mwrite());
    im0_tick(); T(pins_none());
    // interrupt service routine at address 0x0038 (LD A,33)
    im0_tick(); T(pins_m1()); T(cpu.pc == 0x0039);
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_rfsh());
    im0_tick(); T(pins_none());
    // mread
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_mread());
    im0_tick(); T(pins_none());
    // RETI, ED prefix
    im0_tick(); T(pins_m1());
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_rfsh());
    im0_tick(); T(pins_none());
    // RETI opcode
    im0_tick(); T(pins_m1());
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_rfsh());
    im0_tick(); T(pins_none());
    // RETI mread (pop)
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_mread());
    im0_tick(); T(pins_none()); T(pins & Z80_RETI);
    // RETI mread (pop)
    im0_tick(); T(pins_none());
    im0_tick(); T(pins_mread());
    im0_tick(); T(pins_none());

    // continue with next NOP, NOTE: iff1 is *NOT* reenabled!
    im0_tick(); T(pins_m1()); T(!cpu.iff1);
    im0_tick(); T(pins_none()); T(!cpu.iff1);
    im0_tick(); T(pins_rfsh()); T(!cpu.iff1);
    im0_tick(); T(pins_none()); T(!cpu.iff1);
}

// test IM1 interrupt behaviour and timing
UTEST(z80, INT_IM1) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0xED, 0x56,         //      IM 1
        0x00, 0x00, 0x00,   // l0:  NOPs
        0x18, 0xFB,         //      JR l0
    };
    uint8_t isr[] = {
        0x3E, 0x33,         //      LD A,33h
        0xED, 0x4D,         //      RETI
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0038, isr, sizeof(isr));
    cpu.sp = 0x1000;

    // EI + IM1
    skip(12);

    // NOP
    tick(); T(pins_m1()); T(cpu.iff1);
    tick(); T(pins_none());
    pins |= Z80_INT;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());

    // interrupt should trigger now
    tick(); T(pins_none());
    tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);
    tick(); T(pins_m1iorq());
    tick(); T(pins_none());
    // regular refresh cycle
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // one extra tcycle
    tick(); T(pins_none());
    // two mwrite cycles (push PC to stack)
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    // ISR starts here (LD A,33h)
    tick(); T(pins_m1());  T(!cpu.iff1); T(!cpu.iff2); T(cpu.pc == 0x0039);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread (LD A,33h)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());
    // RETI, ED prefix
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // RETI opcode
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // RETI mread (pop)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(pins & Z80_RETI);
    // RETI mread (pop)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());

    // next NOP (interrupts still disabled!)
    tick(); T(pins_m1()); T(!cpu.iff1); T(!cpu.iff2); T(cpu.pc == 5);
}

// test IM2 interrupt behaviour and timing
UTEST(z80, INT_IM2) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0xED, 0x5E,         //      IM 2
        0x3E, 0x01,         //      LD A,1
        0xED, 0x47,         //      LD I,A
        0x00, 0x00, 0x00,   // l0:  NOPs
        0x18, 0xFB,         //      JR l0
        0x00,               //      ---
        0x3E, 0x33,         // isr: LD A,33h
        0xED, 0x4D,         //      RETI
    };
    uint8_t int_vec[] = {
        0x0D, 0x00,         //      DW isr (0x000D)
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x01E0, int_vec, sizeof(int_vec));

    // EI
    skip(4);
    // IM 2
    skip(8);
    // LD A,1
    skip(7);
    // LD I,A
    skip(9); T(cpu.iff1); T(cpu.iff2); T(cpu.im == 2);

    // NOP
    tick(); T(pins_m1()); T(cpu.i == 1);
    tick(); T(pins_none());
    pins |= Z80_INT;
    tick(); T(pins_rfsh());
    tick(); T(pins_none());

    // interrupt should trigger now
    tick(); T(pins_none());
    tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);
    tick(); T(pins_m1iorq());
    tick(); T(pins_none());
    // regular refresh cycle
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // one extra tcycle
    tick(); T(pins_none());
    // two mwrite cycles (push PC to stack)
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none()); T(cpu.wz == 0x01E0);    // WZ is interrupt vector
    // two mread cycles from interrupt vector
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(cpu.wz == 0x000D); T(cpu.pc == 0x000D);

    // ISR starts here (LD A,33h)
    tick(); T(pins_m1()); T(!cpu.iff1); T(!cpu.iff2); T(cpu.pc == 0x000E);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread (LD A,33h)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());
    // RETI, ED prefix
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // RETI opcode
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // RETI mread (pop)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(pins & Z80_RETI);
    // RETI mread (pop)
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());

    // next NOP (interrupts still disabled!)
    tick(); T(pins_m1()); T(!cpu.iff1); T(!cpu.iff2); T(cpu.pc == 9);
}

// maskable interrupts shouldn't trigger when interrupts are disabled
UTEST(z80, INT_disabled) {
    uint8_t prog[] = {
        0xF3,       //      DI
        0xED, 0x56, //      IM 1
        0x00,       // l0:  NOP
        0x18, 0xFD, //      JR l0
    };
    uint8_t isr[] = {
        0x3E, 0x33, //      LD A,33h
        0xED, 0x3D, //      RETI
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0038, isr, sizeof(isr));

    // DI
    skip(4);
    // IM 1
    skip(8);

    // NOP
    tick(); T(pins_m1());
    pins |= Z80_INT;
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // JR l0 (interrupt should not have triggered)
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none());
    // 5x ticks
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_none());

    // next NOP
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
}

// maskable interrupts shouldn't trigger during EI sequences
UTEST(z80, Z80_INT_EI_seq) {
    uint8_t prog[] = {
        0xFB,               //      EI
        0xED, 0x56,         //      IM 1
        0xFB, 0xFB, 0xFB,   // l0:  3x EI
        0x00,               //      NOP
        0x18, 0xFA,         //      JR l0
    };
    uint8_t isr[] = {
        0x3E, 0x33,         //      LD A,33h
        0xED, 0x3D,         //      RETI
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0038, isr, sizeof(isr));

    // EI
    skip(4);
    // IM 1
    skip(8);

    // EI
    tick(); T(pins_m1());
    pins |= Z80_INT;
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // EI (interrupt shouldn't trigger)
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // EI (interrupt shouldn't trigger)
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // NOP (interrupt should trigger at end)
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());

    // IM1 interrupt request starts here
    tick(); T(pins_none());
    tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);
    tick(); T(pins_m1iorq());
    tick(); T(pins_none());
    // regular refresh cycle
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // one extra tcycle
    tick(); T(pins_none());
    // two mwrite cycles (push PC to stack)
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    tick(); T(pins_none());
    tick(); T(pins_mwrite());
    tick(); T(pins_none());
    // ISR starts here (LD A,33h)
    tick(); T(pins_m1());  T(!cpu.iff1); T(!cpu.iff2); T(cpu.pc == 0x0039);
}

UTEST_MAIN()
