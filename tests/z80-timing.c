//------------------------------------------------------------------------------
// z80x-timing.c
//
// Tests tcycle timing.
//
// NOTE: Interrupt-related instructions are tested in z80x-int.c
//------------------------------------------------------------------------------
#include "utest.h"
#define CHIPS_IMPL
#include "chips/z80.h"

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
    tick(); success &= pins_none();
    tick(); success &= pins_mread();
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

static bool ioread_cycle(void) {
    bool success = true;
    tick(); success &= pins_none();
    tick(); success &= pins_none();
    tick(); success &= pins_ioread();
    tick(); success &= pins_none();
    return success;
}

static bool iowrite_cycle(void) {
    bool success = true;
    tick(); success &= pins_none();
    tick(); success &= pins_iowrite();
    tick(); success &= pins_none();
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

static void init(const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    copy(0, bytes, num_bytes);
}

UTEST(z80, LD_r_iHLi_r) {
    uint8_t prog[] = {
        0x7E,       // LD A,(HL)
        0x70,       // LD (HL),B
        0x00, 0x00, // 2x NOP
    };
    init(prog, sizeof(prog));

    // LD A,(HL)
    T(m1_cycle());
    T(mread_cycle());

    // LD (HL),B
    T(m1_cycle());
    T(mwrite_cycle());
    T(finish());
}

UTEST(z80, ALU_iHLi) {
    uint8_t prog[] = {
        0x86,               // ADD A,(HL)
        0xDD, 0x96, 0x01,   // SUB (IX+1)
        0xFD, 0xA6, 0xFF,   // AND (IY-1)
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // ADD A,(HL)
    T(m1_cycle());
    T(mread_cycle());

    // SUB (IX+1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));
    T(mread_cycle());

    // AND (IY-1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));
    T(mread_cycle());

    T(finish());
}

UTEST(z80, NOP) {
    uint8_t prog[] = { 0, 0, 0, 0 };
    init(prog, sizeof(prog));

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
    init(prog, sizeof(prog));

    // LD A,11h
    T(m1_cycle());
    T(mread_cycle());

    // LD B,22h
    T(m1_cycle());
    T(mread_cycle());
    T(finish());
}

UTEST(z80, LD_rp_nn) {
    uint8_t prog[] = {
        0x21, 0x11, 0x11,   // LD HL,1111h
        0x11, 0x22, 0x22,   // LD DE,2222h
        0xDD, 0x21, 0x33, 0x33, // LD IX,3333h
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD HL,1111h
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LD DE,2222h
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LD IX,3333h
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, DJNZ) {
    uint8_t prog[] = {
        0xAF,           //       XOR A
        0x06, 0x03,     //       LD B,3
        0x3C,           // loop: INC a
        0x10, 0xFD,     //       DJNZ loop
        0x00, 0x00
    };
    init(prog, sizeof(prog));

    // XOR A
    T(m1_cycle());
    // LD B,3
    T(m1_cycle());
    T(mread_cycle());
    // INC A
    T(m1_cycle());
    // DJNZ (jump taken)
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(none_cycle(5));
    // INC A
    T(m1_cycle());
    // DJNZ (jump taken)
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(none_cycle(5));
    // INC A
    T(m1_cycle());
    // DJNZ (fallthrough)
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());

    T(finish());
}

UTEST(z80, JR) {
    uint8_t prog[] = {
        0x18, 0x01,     //        JR label
        0x00,           //        NOP
        0x3E, 0x33,     // label: LD A,33h
        0x00, 0x00
    };
    init(prog, sizeof(prog));

    // JR label
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));

    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, JR_cc) {
    uint8_t prog[] = {
        0xAF,           //        XOR A
        0x20, 0x03,     //        JR NZ, label
        0x28, 0x01,     //        JR Z, label
        0x00,           //        NOP
        0x3E, 0x33,     // label: LD A,33h
        0x00, 0x00
    };
    init(prog, sizeof(prog));

    // XOR A
    T(m1_cycle());
    // JR NZ (not taken)
    T(m1_cycle());
    T(mread_cycle());
    // JR Z (taken)
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));
    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());
    T(finish());
}

UTEST(z80, RST) {
    uint8_t prog[] = {
        0xCF,       // RST 8
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x3E, 0x33, // LD A,33h
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // RST 8
    T(m1_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, LD_iHLi_n) {
    uint8_t prog[] = {
        0x36, 0x11,             // LD (HL),11h
        0xDD, 0x36, 0x01, 0x11, // LD (IX+1),11h
        0x00, 0x00,             // NOP NOP
    };
    init(prog, sizeof(prog));

    // LD (HL),n
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());

    // LD (IX+1),11h
    T(m1_cycle());      // DD prefix
    T(m1_cycle());      // 36 opcode
    T(mread_cycle());   // load d-offset
    T(mread_cycle());   // load n
    T(none_cycle(2));   // 2 filler ticks
    T(mwrite_cycle());  // write result

    T(finish());
}

UTEST(z80, LD_r_iIXi_r) {
    uint8_t prog[] = {
        0xDD, 0x7E, 0x01,   // LD A,(IX+1)
        0xFD, 0x70, 0x01,   // LD (IY+1),B
        0x00, 0x00,         // NOP NOP
    };
    init(prog, sizeof(prog));

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

UTEST(z80, LD_A_iBCi_iDEi_inni) {
    uint8_t prog[] = {
        0x0A,   // LD A,(BC)
        0x1A,   // LD A,(DE)
        0x3A, 0x00, 0x10,   // LD A,(1000h)
        0x00, 0x00
    };
    init(prog, sizeof(prog));

    // LD A,(BC)
    T(m1_cycle());
    T(mread_cycle());

    // LD A,(DE)
    T(m1_cycle());
    T(mread_cycle());

    // LD A,(nn)
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, LD_iBCi_iDEi_inni_a) {
    uint8_t prog[] = {
        0x02,       // LD (BC),A
        0x12,       // LD (DE),A
        0x32, 0x00, 0x10,   // LD (1000h),A
    };
    init(prog, sizeof(prog));

    // LD (BC),A
    T(m1_cycle());
    T(mwrite_cycle());

    // LD (DE),A
    T(m1_cycle());
    T(mwrite_cycle());

    // LD (nn),A
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
}

UTEST(z80, LD_iHLi_nn) {
    uint8_t prog[] = {
        0x22, 0x11, 0x11,       // LD (1111h),HL
        0xDD, 0x22, 0x22, 0x22, // LD (2222h),IX
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD (1111h),HL
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(mwrite_cycle());

    // LD (2222h),IX
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(mwrite_cycle());

    T(finish());
}

UTEST(z80, LD_inni_HL) {
    uint8_t prog[] = {
        0x2A, 0x11, 0x11,           // LD (1111h),HL
        0xDD, 0x2A, 0x22, 0x22,     // LD (2222h),IX
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD (nn),HL
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LD (nn),IX
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, INC_DEC_rp) {
    uint8_t prog[] = {
        0x23,       // INC HL
        0x1B,       // DEC DE
        0xDD, 0x23, // INC IX
        0xFD, 0x1B, // DEC IY
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // INC HL
    T(m1_cycle());
    T(none_cycle(2));
    // DEC DE
    T(m1_cycle());
    T(none_cycle(2));
    // INC IX
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(2));
    // DEC IY
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(2));

    T(finish());
}

UTEST(z80, INC_DEC_iHLi) {
    uint8_t prog[] = {
        0x34,               // INC (HL)
        0xDD, 0x35, 0x01,   // DEC (IX+1)
        0xFD, 0x34, 0x02,   // INC (IY+2)
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // INC (HL)
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());

    // DEC (IX+1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());

    // INC (IY+2)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());

    T(finish());
}

UTEST(z80, CALL_RET) {
    uint8_t prog[] = {
        0xCD, 0x08, 0x00,   //      CALL l0
        0xCD, 0x08, 0x00,   //      CALL l1
        0x00, 0x00,         //      NOP NOP
        0xC9                // l0:  RET
    };
    init(prog, sizeof(prog));

    // CALL l0
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // RET
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // CALL l0
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // RET
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, PUSH_POP) {
    uint8_t prog[] = {
        0xE5,           // PUSH HL
        0xDD, 0xE5,     // PUSH IX
        0xE1,           // POP HL
        0xDD, 0xE1,     // POP IX
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // PUSH HL
    T(m1_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // PUSH IX
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // POP HL
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // POP IX
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, LD_SP_HL) {
    uint8_t prog[] = {
        0xF9,       // LD SP,HL
        0xDD, 0xF9, // LD SP,IX
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD SP,HL
    T(m1_cycle());
    T(none_cycle(2));

    // LD SP,IX
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(2));

    T(finish());
}

UTEST(z80, JP_HL) {
    uint8_t prog[] = {
        0x21, 0x05, 0x00,   //      LD HL,l0
        0xE9,               //      JP HL
        0x00,               //      NOP
        0x3E, 0x33,         // l0:  LD A,33h
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD HL,l0
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // JP HL
    T(m1_cycle());

    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, EX_iSPi_HL) {
    uint8_t prog[] = {
        0xE3,           // EX (SP),HL
        0xDD, 0xE3,     // EX (SP),IX
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // EX (SP),HL
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    // EX (SP),IX
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    T(finish());
}

UTEST(z80, JP_nn) {
    uint8_t prog[] = {
        0xC3, 0x04, 0x00,   // JP l0
        0x00,               // NOP
        0x3E, 0x33,         // LD A,33h
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // JP l0
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, CALL_cc_RET_cc) {
    uint8_t prog[] = {
        0x97,               //      SUB A
        0xC4, 0x09, 0x00,   //      CALL NZ,l0
        0xCC, 0x09, 0x00,   //      CALL Z,l0
        0x00,               //      NOP
        0x00,
        0xC0,               // l0:  RET NZ
        0xC8,               //      RET Z
        0x3E, 0x33,         //      LD A,33h
    };
    init(prog, sizeof(prog));

    // SUB A
    T(m1_cycle());

    // CALL NZ, l0 (not taken)
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // CALL Z, l0 (taken)
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());
    T(mwrite_cycle());

    // RET NZ (not taken)
    T(m1_cycle());
    T(none_cycle(1));

    // RET Z (taken)
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, JP_cc_nn) {
    uint8_t prog[] = {
        0x97,               // SUB A
        0xC2, 0x08, 0x00,   // JP NZ, l0
        0xCA, 0x08, 0x00,   // JP Z, l0
        0x00,
        0x3E, 0x33,         // LD A,33h
        0x00, 0x00
    };
    init(prog, sizeof(prog));

    // SUB A
    T(m1_cycle());

    // JP NZ, l0 (not taken)
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // JP Z, l0 (taken)
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LD A,33h
    T(m1_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, IN_OUT_ini_A) {
    uint8_t prog[] = {
        0xD3, 0x33,         // OUT (33h),A
        0xDB, 0x44,         // IN A,(44h)
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // OUT (33h),A
    T(m1_cycle());
    T(mread_cycle());
    T(iowrite_cycle());

    // IN A,(44h)
    T(m1_cycle());
    T(mread_cycle());
    T(ioread_cycle());

    T(finish());
}

UTEST(z80, IN_OUT_ry_iCi) {
    uint8_t prog[] = {
        0xED, 0x78,         // IN A,(C)
        0xED, 0x70,         // IN (C)
        0xED, 0x79,         // OUT (C),A
        0xED, 0x71,         // OUT (C),0
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // IN A,(C)
    T(m1_cycle());
    T(m1_cycle());
    T(ioread_cycle());

    // IN (C)
    T(m1_cycle());
    T(m1_cycle());
    T(ioread_cycle());

    // OUT (C),A
    T(m1_cycle());
    T(m1_cycle());
    T(iowrite_cycle());

    // OUT (C),0
    T(m1_cycle());
    T(m1_cycle());
    T(iowrite_cycle());

    T(finish());
}

UTEST(z80, LD_inni_rp) {
    uint8_t prog[] = {
        0xED, 0x43, 0x11, 0x11,     // LD (1111h),BC
        0xED, 0x4B, 0x22, 0x22,     // LD BC,(2222h)
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD (1111h),BC
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(mwrite_cycle());

    // LD BC,(2222h)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(mread_cycle());

    T(finish());
}

UTEST(z80, RRD_RLD) {
    uint8_t prog[] = {
        0xED, 0x67,     // RRD
        0xED, 0x6F,     // RLD
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // RRD
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(4));
    T(mwrite_cycle());

    // RLD
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(4));
    T(mwrite_cycle());

    T(finish());
}

UTEST(z80, LDI_LDD_CPI_CPD) {
    uint8_t prog[] = {
        0xED, 0xA0,     // LDI
        0xED, 0xA8,     // LDD
        0xED, 0xA1,     // CPI
        0xED, 0xA9,     // CPD
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LDI
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    // LDD
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    // CPI
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));

    // CPD
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));

    T(finish());
}

UTEST(z80, INI_IND_OUTI_OUTD) {
    uint8_t prog[] = {
        0xED, 0xA2,     // INI
        0xED, 0xAA,     // IND
        0xED, 0xA3,     // OUTI
        0xED, 0xAB,     // OUTD
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // INI
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());

    // IND
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());

    // OUTI
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());

    // OUTD
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());

    T(finish());
}

UTEST(z80, LDIR_LDDR) {
    uint8_t prog[] = {
        0x01, 0x02, 0x00,   // LD BC,2
        0xED, 0xB0,         // LDIR
        0x01, 0x02, 0x00,   // LD BC,2
        0xED, 0xB8,         // LDDR
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LDIR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(7));

    // LDIR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // LDDR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(7));

    // LDDR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mwrite_cycle());
    T(none_cycle(2));

    T(finish());
}

UTEST(z80, CPIR_CPDR) {
    uint8_t prog[] = {
        0x01, 0x02, 0x00,   // LD BC,2
        0xED, 0xB1,         // CPIR
        0x01, 0x02, 0x00,   // LD BC,2
        0xED, 0xB9,         // CPDR
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // CPIR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(10));

    // CPIR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());

    // CPDR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(10));

    // CPDR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(5));

    T(finish());
}

UTEST(z80, INIR_INDR) {
    uint8_t prog[] = {
        0x06, 0x02,         // LD B,2
        0xED, 0xB2,         // INIR
        0x06, 0x02,         // LD B,2
        0xED, 0xBA,         // INDR
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());

    // INIR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());
    T(none_cycle(5));

    // INIR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());

    // LD BC,2
    T(m1_cycle());
    T(mread_cycle());

    // INDR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());
    T(none_cycle(5));

    // INDR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(ioread_cycle());
    T(mwrite_cycle());

    T(finish());
}

UTEST(z80, OTIR_OTDR) {
    uint8_t prog[] = {
        0x06, 0x02,     // LD B,2
        0xED, 0xB3,     // OTIR
        0x06, 0x02,     // LD B,2
        0xED, 0xBB,     // OTDR
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // LD B,2
    T(m1_cycle());
    T(mread_cycle());

    // OTIR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());
    T(none_cycle(5));

    // OTIR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());

    // LD B,2
    T(m1_cycle());
    T(mread_cycle());

    // OTDR (1)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());
    T(none_cycle(5));

    // OTDR (2)
    T(m1_cycle());
    T(m1_cycle());
    T(none_cycle(1));
    T(mread_cycle());
    T(iowrite_cycle());

    T(finish());
}

UTEST(z80, SET_BIT_n_r) {
    uint8_t prog[] = {
        0xCB, 0xC7,     // SET 0,A
        0xCB, 0x48,     // BIT 1,B
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // SET 0,A
    T(m1_cycle());
    T(m1_cycle());

    // BIT 1,B
    T(m1_cycle());
    T(m1_cycle());

    T(finish());
}

UTEST(z80, SET_BIT_iHLi) {
    uint8_t prog[] = {
        0xCB, 0xC6,                 // SET 0,(HL)
        0xDD, 0xCB, 0x01, 0xC6,     // SET 0,(IX+1)
        0xCB, 0x46,                 // BIT 0,(HL)
        0xDD, 0xCB, 0x01, 0x46,     // BIT 0,(IX+1)
        0x00, 0x00,
    };
    init(prog, sizeof(prog));

    // SET 0,(HL)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());

    // SET 0,(IX+1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(2));
    T(mread_cycle());
    T(none_cycle(1));
    T(mwrite_cycle());

    // BIT 0,(HL)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(none_cycle(1));

    // BIT 0,(IX+1)
    T(m1_cycle());
    T(m1_cycle());
    T(mread_cycle());
    T(mread_cycle());
    T(none_cycle(2));
    T(mread_cycle());
    T(none_cycle(1));

    T(finish());
}

UTEST_MAIN()
