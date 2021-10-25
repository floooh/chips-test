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

static bool pins_none(void) {
    return (pins & Z80_CTRL_PIN_MASK) == 0;
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

static bool pins_ioread(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_IORQ|Z80_RD);
}

static bool pins_iowrite(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_IORQ|Z80_WR);
}

static bool none_cycle(int num) {
    bool success = true;
    for (int i = 0; i < num; i++) {
        tick(); success &= pins_none();
    }
    return success;
}

static bool m1_cycle(void) {
    bool success = true;
    tick(); success &= pins_m1();
    tick(); success &= pins_none();
    tick(); success &= pins_rfsh();
    tick(); success &= pins_none();
    return success;
}

static bool mread_cycle(void) {
    bool success = true;
    tick(); success &= pins_mread();
    tick(); success &= pins_none();
    tick(); success &= pins_none();
    return success;
}

static bool mwrite_cycle(void) {
    bool success = true;
    tick(); success &= pins_none();
    tick(); success &= pins_mwrite();
    tick(); success &= pins_none();
    return success;
}

static bool finish(void) {
    // run 2x NOP
    for (int i = 0; i < 2; i++) {
        tick(); if (!pins_m1()) { return false; }
        tick(); if (!pins_none()) { return false; }
        tick(); if (!pins_rfsh()) { return false; }
        tick(); if (!pins_none()) { return false; }
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

    // 2x NOP
    T(m1_cycle());
    T(m1_cycle());
}

UTEST(z80, LD_r_n) {
    uint8_t prog[] = {
        0x3E, 0x11,     // LD A,11h
        0x06, 0x22,     // LD B,22h
        0x00, 0x00,     // NOP, NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD A,11h
    T(m1_cycle());
    T(mread_cycle());

    // LD B,22h
    T(m1_cycle());
    T(mread_cycle());
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
    T(m1_cycle());
    T(mread_cycle());

    // LD (HL),B
    T(m1_cycle());
    T(mwrite_cycle());
    T(finish());
}

UTEST(z80, LD_iHLi_n) {
    uint8_t prog[] = {
        0x36, 0x11,         // LD (HL),11h
        0x00, 0x00,         // NOP NOP
    };
    init(0x0000, prog, sizeof(prog));

    // LD (HL),n
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
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
    T(m1_cycle());      // DD prefix
    T(m1_cycle());      // 7E opcode
    T(mread_cycle());   // load d-offset
    T(none_cycle(5));   // filler ticks
    T(mread_cycle());   // load (IX+1)

    // LD (IY+1),B
    T(m1_cycle());      // FD prefix
    T(m1_cycle());      // 70 opcode
    T(mread_cycle());   // load d-offset
    T(none_cycle(5));   // filler ticks
    T(mwrite_cycle());  // write (IY+1)

    T(finish());
}

UTEST(z80, LD_iIXi_n) {
    uint8_t prog[] = {
        0xDD, 0x36, 0x01, 0x11, // LD (IX+1),11h
        0x00, 0x00
    };
    init(0x0000, prog, sizeof(prog));

    // LD (IX+1),11h
    T(m1_cycle());      // DD prefix
    T(m1_cycle());      // 36 opcode
    T(mread_cycle());   // load d-offset
    T(mread_cycle());   // load n
    T(none_cycle(2));   // 2 filler ticks
    T(mwrite_cycle());  // write result

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
    T(m1_cycle());      // DD prefix
    T(m1_cycle());      // CB prefix
    T(mread_cycle());   // load d-offset
    T(mread_cycle());   // C6 opcode
    T(none_cycle(2));   // 2 filler ticks
    T(mread_cycle());   // load (ix+1)
    T(none_cycle(1));   // 1 filler tick
    T(mwrite_cycle());  // write result

    // SET 1,(iy+1)
    T(m1_cycle());      // DD prefix
    T(m1_cycle());      // CB prefix
    T(mread_cycle());   // load d-offset
    T(mread_cycle());   // CE opcode
    T(none_cycle(2));   // 2 filler ticks
    T(mread_cycle());   // load (iy+1)
    T(none_cycle(1));   // 1 filler tick
    T(mwrite_cycle());  // write result

    T(finish());
}

UTEST_MAIN()
