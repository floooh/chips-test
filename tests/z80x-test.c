//------------------------------------------------------------------------------
//  z80x-test.c
//------------------------------------------------------------------------------
#include "utest.h"
#define CHIPS_IMPL
#include "chips/z80x.h"

#define T(b) ASSERT_TRUE(b)

#define _A (cpu.a)
#define _F (cpu.f)
#define _L (cpu.l)
#define _H (cpu.h)
#define _E (cpu.e)
#define _D (cpu.d)
#define _C (cpu.c)
#define _B (cpu.b)
#define _WZ (cpu.wz)
#define _FA ((uint16_t)(cpu.f<<8)|cpu.a)
#define _HL ((uint16_t)(cpu.h<<8)|cpu.l) 
#define _DE ((uint16_t)(cpu.d<<8)|cpu.e)
#define _BC ((uint16_t)(cpu.b<<8)|cpu.c)

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

static uint32_t step(void) {
    uint32_t ticks = 0;
    do {
        tick();
        ticks++;
    } while (!z80_opdone(&cpu));
    return ticks;
}

static void prefetch(uint16_t pc) {
    pins = Z80_M1|Z80_MREQ|Z80_RD;
    Z80_SET_ADDR(pins, pc);
    Z80_SET_DATA(pins, mem[pc]);
    cpu.pc = (pc + 1) & 0xFFFF;
    cpu.f = 0;
    // advance to the first refresh cycle
    step();
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    copy(start_addr, bytes, num_bytes);
    prefetch(start_addr);    
}

/* LD r,s; LD r,n */
UTEST(z80, LD_r_sn) {
    uint8_t prog[] = {
        0x3E, 0x12,     // LD A,0x12
        0x47,           // LD B,A
        0x4F,           // LD C,A
        0x57,           // LD D,A
        0x5F,           // LD E,A
        0x67,           // LD H,A
        0x6F,           // LD L,A
        0x7F,           // LD A,A

        0x06, 0x13,     // LD B,0x13
        0x40,           // LD B,B
        0x48,           // LD C,B
        0x50,           // LD D,B
        0x58,           // LD E,B
        0x60,           // LD H,B
        0x68,           // LD L,B
        0x78,           // LD A,B

        0x0E, 0x14,     // LD C,0x14
        0x41,           // LD B,C
        0x49,           // LD C,C
        0x51,           // LD D,C
        0x59,           // LD E,C
        0x61,           // LD H,C
        0x69,           // LD L,C
        0x79,           // LD A,C

        0x16, 0x15,     // LD D,0x15
        0x42,           // LD B,D
        0x4A,           // LD C,D
        0x52,           // LD D,D
        0x5A,           // LD E,D
        0x62,           // LD H,D
        0x6A,           // LD L,D
        0x7A,           // LD A,D

        0x1E, 0x16,     // LD E,0x16
        0x43,           // LD B,E
        0x4B,           // LD C,E
        0x53,           // LD D,E
        0x5B,           // LD E,E
        0x63,           // LD H,E
        0x6B,           // LD L,E
        0x7B,           // LD A,E

        0x26, 0x17,     // LD H,0x17
        0x44,           // LD B,H
        0x4C,           // LD C,H
        0x54,           // LD D,H
        0x5C,           // LD E,H
        0x64,           // LD H,H
        0x6C,           // LD L,H
        0x7C,           // LD A,H

        0x2E, 0x18,     // LD L,0x18
        0x45,           // LD B,L
        0x4D,           // LD C,L
        0x55,           // LD D,L
        0x5D,           // LD E,L
        0x65,           // LD H,L
        0x6D,           // LD L,L
        0x7D,           // LD A,L
    };
    init(0x0000, prog, sizeof(prog));

    T(7==step()); T(0x12==_A);
    T(4==step()); T(0x12==_B);
    T(4==step()); T(0x12==_C);
    T(4==step()); T(0x12==_D);
    T(4==step()); T(0x12==_E);
    T(4==step()); T(0x12==_H);
    T(4==step()); T(0x12==_L);
    T(4==step()); T(0x12==_A);
    T(7==step()); T(0x13==_B);
    T(4==step()); T(0x13==_B);
    T(4==step()); T(0x13==_C);
    T(4==step()); T(0x13==_D);
    T(4==step()); T(0x13==_E);
    T(4==step()); T(0x13==_H);
    T(4==step()); T(0x13==_L);
    T(4==step()); T(0x13==_A);
    T(7==step()); T(0x14==_C);
    T(4==step()); T(0x14==_B);
    T(4==step()); T(0x14==_C);
    T(4==step()); T(0x14==_D);
    T(4==step()); T(0x14==_E);
    T(4==step()); T(0x14==_H);
    T(4==step()); T(0x14==_L);
    T(4==step()); T(0x14==_A);
    T(7==step()); T(0x15==_D);
    T(4==step()); T(0x15==_B);
    T(4==step()); T(0x15==_C);
    T(4==step()); T(0x15==_D);
    T(4==step()); T(0x15==_E);
    T(4==step()); T(0x15==_H);
    T(4==step()); T(0x15==_L);
    T(4==step()); T(0x15==_A);
    T(7==step()); T(0x16==_E);
    T(4==step()); T(0x16==_B);
    T(4==step()); T(0x16==_C);
    T(4==step()); T(0x16==_D);
    T(4==step()); T(0x16==_E);
    T(4==step()); T(0x16==_H);
    T(4==step()); T(0x16==_L);
    T(4==step()); T(0x16==_A);
    T(7==step()); T(0x17==_H);
    T(4==step()); T(0x17==_B);
    T(4==step()); T(0x17==_C);
    T(4==step()); T(0x17==_D);
    T(4==step()); T(0x17==_E);
    T(4==step()); T(0x17==_H);
    T(4==step()); T(0x17==_L);
    T(4==step()); T(0x17==_A);
    T(7==step()); T(0x18==_L);
    T(4==step()); T(0x18==_B);
    T(4==step()); T(0x18==_C);
    T(4==step()); T(0x18==_D);
    T(4==step()); T(0x18==_E);
    T(4==step()); T(0x18==_H);
    T(4==step()); T(0x18==_L);
    T(4==step()); T(0x18==_A);
}

/* LD r,(HL) */
UTEST(z80, LD_r_iHLi) {
    uint8_t prog[] = {
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x3E, 0x33,         // LD A,0x33
        0x77,               // LD (HL),A
        0x3E, 0x22,         // LD A,0x22
        0x46,               // LD B,(HL)
        0x4E,               // LD C,(HL)
        0x56,               // LD D,(HL)
        0x5E,               // LD E,(HL)
        0x66,               // LD H,(HL)
        0x26, 0x10,         // LD H,0x10
        0x6E,               // LD L,(HL)
        0x2E, 0x00,         // LD L,0x00
        0x7E,               // LD A,(HL)
    };
    init(0x0000, prog, sizeof(prog));

    T(10==step()); T(0x1000 == _HL);
    T(7==step()); T(0x33 == _A);
    T(7==step()); T(0x33 == mem[0x1000]);
    T(7==step()); T(0x22 == _A);
    T(7==step()); T(0x33 == _B);
    T(7==step()); T(0x33 == _C);
    T(7==step()); T(0x33 == _D);
    T(7==step()); T(0x33 == _E);
    T(7==step()); T(0x33 == _H);
    T(7==step()); T(0x10 == _H);
    T(7==step()); T(0x33 == _L);
    T(7==step()); T(0x00 == _L);
    T(7==step()); T(0x33 == _A);       
}

/* LD (HL),r */
UTEST(z80, LD_iHLi_r) {
    uint8_t prog[] = {
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x3E, 0x12,         // LD A,0x12
        0x77,               // LD (HL),A
        0x06, 0x13,         // LD B,0x13
        0x70,               // LD (HL),B
        0x0E, 0x14,         // LD C,0x14
        0x71,               // LD (HL),C
        0x16, 0x15,         // LD D,0x15
        0x72,               // LD (HL),D
        0x1E, 0x16,         // LD E,0x16
        0x73,               // LD (HL),E
        0x74,               // LD (HL),H
        0x75,               // LD (HL),L
    };
    init(0x0000, prog, sizeof(prog));

    T(10==step()); T(0x1000 == _HL);
    T(7==step()); T(0x12 == _A);
    T(7==step()); T(0x12 == mem[0x1000]);
    T(7==step()); T(0x13 == _B);
    T(7==step()); T(0x13 == mem[0x1000]);
    T(7==step()); T(0x14 == _C);
    T(7==step()); T(0x14 == mem[0x1000]);
    T(7==step()); T(0x15 == _D);
    T(7==step()); T(0x15 == mem[0x1000]);
    T(7==step()); T(0x16 == _E);
    T(7==step()); T(0x16 == mem[0x1000]);
    T(7==step()); T(0x10 == mem[0x1000]);
    T(7==step()); T(0x00 == mem[0x1000]);
}

/* LD (HL),n */
UTEST(z80, LD_iHLi_n) {
    uint8_t prog[] = {
        0x21, 0x00, 0x20,   // LD HL,0x2000
        0x36, 0x33,         // LD (HL),0x33
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x36, 0x65,         // LD (HL),0x65
    };
    init(0x0000, prog, sizeof(prog));

    T(10==step()); T(0x2000 == _HL);
    T(10==step()); T(0x33 == mem[0x2000]);
    T(10==step()); T(0x1000 == _HL);
    T(10==step()); T(0x65 == mem[0x1000]);
}

/* LD A,(BC); LD A,(DE); LD A,(nn) */
UTEST(z80, LD_A_iBCDEnni) {
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x0A,               // LD A,(BC)
        0x1A,               // LD A,(DE)
        0x3A, 0x02, 0x10,   // LD A,(0x1002)
    };
    init(0x0000, prog, sizeof(prog));
    uint8_t data[] = {
        0x11, 0x22, 0x33
    };
    copy(0x1000, data, sizeof(data));

    T(10==step()); T(0x1000 == _BC);
    T(10==step()); T(0x1001 == _DE);
    T(7 ==step()); T(0x11 == _A); T(0x1001 == _WZ);
    T(7 ==step()); T(0x22 == _A); T(0x1002 == _WZ);
    T(13==step()); T(0x33 == _A); T(0x1003 == _WZ);
}

UTEST_MAIN()



