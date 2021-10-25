//------------------------------------------------------------------------------
//  z80x-timing.c
//
//  Tests tcycle timing.
//------------------------------------------------------------------------------
#include "utest.h"
#define CHIPS_IMPL
#include "chips/z80x.h"

#define T(b) ASSERT_TRUE(b)

static z80_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16];
static uint8_t io[1<<16];

static void tick(void) {
    pins = z80_tick(&cpu, pins);
    const uint16_t addr = Z80_GET_ADDR(pins);
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, io[addr]);
        }
        else if (pins & Z80_WR) {
            io[addr] = Z80_GET_DATA(pins);
        }
    }
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static bool none(void) {
    return (pins & Z80_CTRL_PIN_MASK) == 0;
}

static bool m1(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_M1|Z80_MREQ|Z80_RD);
}

static bool rfsh(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RFSH);
}

static bool mread(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RD);
}

static bool mwrite(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_WR);
}

static bool ioread(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_IORQ|Z80_RD);
}

static bool iowrite(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_IORQ|Z80_WR);
}

static bool finish(void) {
    // run 2x NOP
    for (int i = 0; i < 2; i++) {
        tick(); if (!m1()) { return false; }
        tick(); if (!none()) { return false; }
        tick(); if (!rfsh()) { return false; }
        tick(); if (!none()) { return false; }
    }
    return true;
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    copy(start_addr, bytes, num_bytes);
    // run through the initial NOP
    tick();
    tick();
    tick();
}

UTEST(z80, NOP) {
    uint8_t prog[] = { 0, 0, 0, 0 };
    init(0x0000, prog, sizeof(prog));

    // NOP
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    // NOP
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());
}

UTEST(z80, LD_r_n) {
    uint8_t prog[] = {
        0x3E, 0x11,     // LD A,11h
        0x06, 0x22,     // LD B,22h
        0x00, 0x00,     // NOP, NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD A,11h
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    tick(); T(mread());
    tick(); T(none());
    tick(); T(none());

    // LD B,22h
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    tick(); T(mread());
    tick(); T(none());
    tick(); T(none());

    T(finish());
}

UTEST(z80, LD_r_iHLi_r) {
    uint8_t prog[] = {
        0x7E,       // LD A,(HL)
        0x70,       // LD (HL),B
        0x00, 0x00, // 2x NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD A,(HL)
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    tick(); T(mread());
    tick(); T(none());
    tick(); T(none());

    // LD (HL),B
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    tick(); T(none());
    tick(); T(mwrite());
    tick(); T(none());

    T(finish());
}

UTEST(z80, LD_iHLi_n) {
    uint8_t prog[] = {
        0x36, 0x11,         // LD (HL),11h
        0x00, 0x00,         // NOP NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD (HL),n
    tick(); T(m1());
    tick(); T(none());
    tick(); T(rfsh()); 
    tick(); T(none());

    tick(); T(mread());
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());
    tick(); T(mwrite());
    tick(); T(none());

    T(finish());
}

UTEST(z80, LD_r_iIXi_r) {
    uint8_t prog[] = {
        0xDD, 0x7E, 0x01,   // LD A,(IX+1)
        0xFD, 0x70, 0x01,   // LD (IY+1),B
        0x00, 0x00,         // NOP NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD A,(IX+1)
    tick(); T(m1());        // DD prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(m1());        // opcode
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(mread());     // load d-offset
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 5x filler ticks
    tick(); T(none());
    tick(); T(none());
    tick(); T(none());
    tick(); T(none());

    tick(); T(mread());     // load (IX+1)
    tick(); T(none());
    tick(); T(none());

    // LD (IY+1),B
    tick(); T(m1());        // DD prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(m1());        // opcode
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(mread());     // load d-offset
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 5x filler ticks
    tick(); T(none());
    tick(); T(none());
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // write (IY+1)
    tick(); T(mwrite());
    tick(); T(none());
    
    T(finish());
}

UTEST(z80, LD_iIXi_n) {
    uint8_t prog[] = {
        0xDD, 0x36, 0x01, 0x11, // LD (IX+1),11h
        0x00, 0x00
    };
    init(0x0000, prog, sizeof(prog));

    // LD (IX+1),11h
    tick(); T(m1());        // DD prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(m1());        // opcode
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(mread());     // load d-offset
    tick(); T(none());
    tick(); T(none());

    tick(); T(mread());     // load n
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 2x filler ticks
    tick(); T(none());

    tick(); T(none());      // write result
    tick(); T(mwrite());
    tick(); T(none());

    T(finish());
}

UTEST(z80, SET_IX) {
    uint8_t prog[] = {
        0xDD, 0xCB, 0x01, 0xC6, // SET 0,(ix+1)
        0xFD, 0xCB, 0x01, 0xCE, // SET 1,(iy+1)
        0x00, 0x00,             // NOP NOP
    };
    init(0x0000, prog, sizeof(prog));

    // SET 0,(ix+1)
    tick(); T(m1());        // DD prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());
    
    tick(); T(m1());        // CB prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());
    
    tick(); T(mread());     // d-offset
    tick(); T(none());
    tick(); T(none());

    tick(); T(mread());     // load opcode
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 2x filler ticks
    tick(); T(none());

    tick(); T(mread());     // load (ix+1)
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 1x filler tick

    tick(); T(none());
    tick(); T(mwrite());
    tick(); T(none());    // write result back

    // SET 1,(iy+1)
    tick(); T(m1());        // DD prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(m1());        // CB prefix
    tick(); T(none());
    tick(); T(rfsh());
    tick(); T(none());

    tick(); T(mread());     // d-offset
    tick(); T(none());
    tick(); T(none());

    tick(); T(mread());     // load opcode
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 2x filler ticks
    tick(); T(none());

    tick(); T(mread());     // load (ix+1)
    tick(); T(none());
    tick(); T(none());

    tick(); T(none());      // 1x filler tick

    tick(); T(none());
    tick(); T(mwrite());
    tick(); T(none());    // write result back

    T(finish());
}

UTEST_MAIN()
