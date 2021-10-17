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
#define _FA (cpu.af)
#define _HL (cpu.hl)
#define _DE (cpu.de)
#define _BC (cpu.bc)
#define _IX (cpu.ix)
#define _IY (cpu.iy)

static z80_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16];
static uint8_t io[1<<16];

static bool flags(uint8_t expected) {
    // don't check undocumented flags
    return (cpu.f & ~(Z80_YF|Z80_XF)) == expected;
}

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
    pins = z80_prefetch(&cpu, pc);
    step();
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    cpu.f = 0;
    copy(start_addr, bytes, num_bytes);
    prefetch(start_addr);    
}

// test that the nested ix/iy/hl mapping union works
UTEST(z80, MAP_IX_IY_HL) {
    z80_init(&cpu);
    cpu.hl = 0x1122;
    cpu.ix = 0x3344;
    cpu.iy = 0x5566;
    T(cpu.h == 0x11);
    T(cpu.l == 0x22);
    T(cpu.ixh == 0x33);
    T(cpu.ixl == 0x44);
    T(cpu.iyh == 0x55);
    T(cpu.iyl == 0x66);
    T(cpu.hlx[0].h == 0x11);
    T(cpu.hlx[0].l == 0x22);
    T(cpu.hlx[1].h == 0x33);
    T(cpu.hlx[1].l == 0x44);
    T(cpu.hlx[2].h == 0x55);
    T(cpu.hlx[2].l == 0x66);
    T(cpu.hlx[0].hl == 0x1122);
    T(cpu.hlx[1].hl == 0x3344);
    T(cpu.hlx[2].hl == 0x5566);
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

/* LD (IX|IY+d),r */
UTEST(z80, LD_iIXIYi_r) {
    uint8_t prog[] = {
        0xDD, 0x21, 0x03, 0x10,     // LD IX,0x1003
        0x3E, 0x12,                 // LD A,0x12
        0xDD, 0x77, 0x00,           // LD (IX+0),A
        0x06, 0x13,                 // LD B,0x13
        0xDD, 0x70, 0x01,           // LD (IX+1),B
        0x0E, 0x14,                 // LD C,0x14
        0xDD, 0x71, 0x02,           // LD (IX+2),C
        0x16, 0x15,                 // LD D,0x15
        0xDD, 0x72, 0xFF,           // LD (IX-1),D
        0x1E, 0x16,                 // LD E,0x16
        0xDD, 0x73, 0xFE,           // LD (IX-2),E
        0x26, 0x17,                 // LD H,0x17
        0xDD, 0x74, 0x03,           // LD (IX+3),H
        0x2E, 0x18,                 // LD L,0x18
        0xDD, 0x75, 0xFD,           // LD (IX-3),L
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x3E, 0x12,                 // LD A,0x12
        0xFD, 0x77, 0x00,           // LD (IY+0),A
        0x06, 0x13,                 // LD B,0x13
        0xFD, 0x70, 0x01,           // LD (IY+1),B
        0x0E, 0x14,                 // LD C,0x14
        0xFD, 0x71, 0x02,           // LD (IY+2),C
        0x16, 0x15,                 // LD D,0x15
        0xFD, 0x72, 0xFF,           // LD (IY-1),D
        0x1E, 0x16,                 // LD E,0x16
        0xFD, 0x73, 0xFE,           // LD (IY-2),E
        0x26, 0x17,                 // LD H,0x17
        0xFD, 0x74, 0x03,           // LD (IY+3),H
        0x2E, 0x18,                 // LD L,0x18
        0xFD, 0x75, 0xFD,           // LD (IY-3),L
    };
    init(0x0000, prog, sizeof(prog));
    T(14==step()); T(0x1003 == _IX);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]); T(_WZ == 0x1003);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]); T(_WZ == 0x1004);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]); T(_WZ == 0x1005);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]); T(_WZ == 0x1002);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]); T(_WZ == 0x1001);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]); T(_WZ == 0x1006);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]); T(_WZ == 0x1000);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]); T(_WZ == 0x1003);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]); T(_WZ == 0x1004);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]); T(_WZ == 0x1005);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]); T(_WZ == 0x1002);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]); T(_WZ == 0x1001);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]); T(_WZ == 0x1006);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]); T(_WZ == 0x1000);
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

/* LD (BC),A; LD (DE),A; LD (nn),A */
UTEST(z80, LD_iBCDEnni_A) {
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x3E, 0x77,         // LD A,0x77
        0x02,               // LD (BC),A
        0x12,               // LD (DE),A
        0x32, 0x02, 0x10,   // LD (0x1002),A
    };
    init(0x0000, prog, sizeof(prog));
    T(10==step()); T(0x1000 == _BC);
    T(10==step()); T(0x1001 == _DE);
    T(7 ==step()); T(0x77 == _A);
    T(7 ==step()); T(0x77 == mem[0x1000]); T(0x7701 == _WZ);
    T(7 ==step()); T(0x77 == mem[0x1001]); T(0x7702 == _WZ);
    T(13==step()); T(0x77 == mem[0x1002]); T(0x7703 == _WZ);
}

/* ADD A,r; ADD A,n */
UTEST(z80, ADD_A_rn) {
    uint8_t prog[] = {
        0x3E, 0x0F,     // LD A,0x0F
        0x87,           // ADD A,A
        0x06, 0xE0,     // LD B,0xE0
        0x80,           // ADD A,B
        0x3E, 0x81,     // LD A,0x81
        0x0E, 0x80,     // LD C,0x80
        0x81,           // ADD A,C
        0x16, 0xFF,     // LD D,0xFF
        0x82,           // ADD A,D
        0x1E, 0x40,     // LD E,0x40
        0x83,           // ADD A,E
        0x26, 0x80,     // LD H,0x80
        0x84,           // ADD A,H
        0x2E, 0x33,     // LD L,0x33
        0x85,           // ADD A,L
        0xC6, 0x44,     // ADD A,0x44
    };
    init(0x0000, prog, sizeof(prog));
    T(7==step()); T(0x0F == _A); T(flags(0));
    T(4==step()); T(0x1E == _A); T(flags(Z80_HF));
    T(7==step()); T(0xE0 == _B);
    T(4==step()); T(0xFE == _A); T(flags(Z80_SF));
    T(7==step()); T(0x81 == _A);
    T(7==step()); T(0x80 == _C);
    T(4==step()); T(0x01 == _A); T(flags(Z80_VF|Z80_CF));
    T(7==step()); T(0xFF == _D);
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_HF|Z80_CF));
    T(7==step()); T(0x40 == _E);
    T(4==step()); T(0x40 == _A); T(flags(0));
    T(7==step()); T(0x80 == _H);
    T(4==step()); T(0xC0 == _A); T(flags(Z80_SF));
    T(7==step()); T(0x33 == _L);
    T(4==step()); T(0xF3 == _A); T(flags(Z80_SF));
    T(7==step()); T(0x37 == _A); T(flags(Z80_CF));
}

/* ADC A,r; ADC A,n */
UTEST(z80, ADC_A_rn) {
    uint8_t prog[] = {
        0x3E, 0x00,         // LD A,0x00
        0x06, 0x41,         // LD B,0x41
        0x0E, 0x61,         // LD C,0x61
        0x16, 0x81,         // LD D,0x81
        0x1E, 0x41,         // LD E,0x41
        0x26, 0x61,         // LD H,0x61
        0x2E, 0x81,         // LD L,0x81
        0x8F,               // ADC A,A
        0x88,               // ADC A,B
        0x89,               // ADC A,C
        0x8A,               // ADC A,D
        0x8B,               // ADC A,E
        0x8C,               // ADC A,H
        0x8D,               // ADC A,L
        0xCE, 0x01,         // ADC A,0x01
    };
    init(0x0000, prog, sizeof(prog));
    T(7==step()); T(0x00 == _A);
    T(7==step()); T(0x41 == _B);
    T(7==step()); T(0x61 == _C);
    T(7==step()); T(0x81 == _D);
    T(7==step()); T(0x41 == _E);
    T(7==step()); T(0x61 == _H);
    T(7==step()); T(0x81 == _L);
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF));
    T(4==step()); T(0x41 == _A); T(flags(0));
    T(4==step()); T(0xA2 == _A); T(flags(Z80_SF|Z80_VF));
    T(4==step()); T(0x23 == _A); T(flags(Z80_VF|Z80_CF));
    T(4==step()); T(0x65 == _A); T(flags(0));
    T(4==step()); T(0xC6 == _A); T(flags(Z80_SF|Z80_VF));
    T(4==step()); T(0x47 == _A); T(flags(Z80_VF|Z80_CF));
    T(7==step()); T(0x49 == _A); T(flags(0));
}

/* SUB A,r; SUB A,n */
UTEST(z80, SUB_A_rn) {
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x01,     // LD B,0x01
        0x0E, 0xF8,     // LD C,0xF8
        0x16, 0x0F,     // LD D,0x0F
        0x1E, 0x79,     // LD E,0x79
        0x26, 0xC0,     // LD H,0xC0
        0x2E, 0xBF,     // LD L,0xBF
        0x97,           // SUB A,A
        0x90,           // SUB A,B
        0x91,           // SUB A,C
        0x92,           // SUB A,D
        0x93,           // SUB A,E
        0x94,           // SUB A,H
        0x95,           // SUB A,L
        0xD6, 0x01,     // SUB A,0x01
        0xD6, 0xFE,     // SUB A,0xFE
    };
    init(0x0000, prog, sizeof(prog));
    T(7==step()); T(0x04 == _A);
    T(7==step()); T(0x01 == _B);
    T(7==step()); T(0xF8 == _C);
    T(7==step()); T(0x0F == _D);
    T(7==step()); T(0x79 == _E);
    T(7==step()); T(0xC0 == _H);
    T(7==step()); T(0xBF == _L);
    T(4==step()); T(0x0 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x07 == _A); T(flags(Z80_NF));
    T(4==step()); T(0xF8 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x7F == _A); T(flags(Z80_HF|Z80_VF|Z80_NF));
    T(4==step()); T(0xBF == _A); T(flags(Z80_SF|Z80_VF|Z80_NF|Z80_CF));
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(7==step()); T(0x01 == _A); T(flags(Z80_NF));
}

/* SBC A,r; SBC A,n */
UTEST(z80, SBC_A_rn) {
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x01,     // LD B,0x01
        0x0E, 0xF8,     // LD C,0xF8
        0x16, 0x0F,     // LD D,0x0F
        0x1E, 0x79,     // LD E,0x79
        0x26, 0xC0,     // LD H,0xC0
        0x2E, 0xBF,     // LD L,0xBF
        0x97,           // SUB A,A
        0x98,           // SBC A,B
        0x99,           // SBC A,C
        0x9A,           // SBC A,D
        0x9B,           // SBC A,E
        0x9C,           // SBC A,H
        0x9D,           // SBC A,L
        0xDE, 0x01,     // SBC A,0x01
        0xDE, 0xFE,     // SBC A,0xFE
    };
    init(0x0000, prog, sizeof(prog));
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(4==step()); T(0x0 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x06 == _A); T(flags(Z80_NF));
    T(4==step()); T(0xF7 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x7D == _A); T(flags(Z80_HF|Z80_VF|Z80_NF));
    T(4==step()); T(0xBD == _A); T(flags(Z80_SF|Z80_VF|Z80_NF|Z80_CF));
    T(4==step()); T(0xFD == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(7==step()); T(0xFB == _A); T(flags(Z80_SF|Z80_NF));
    T(7==step()); T(0xFD == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
}

/* CP A,r; CP A,n */
UTEST(z80, CP_A_rn) {
    uint8_t prog[] = {
        0x3E, 0x04,     // LD A,0x04
        0x06, 0x05,     // LD B,0x05
        0x0E, 0x03,     // LD C,0x03
        0x16, 0xff,     // LD D,0xff
        0x1E, 0xaa,     // LD E,0xaa
        0x26, 0x80,     // LD H,0x80
        0x2E, 0x7f,     // LD L,0x7f
        0xBF,           // CP A
        0xB8,           // CP B
        0xB9,           // CP C
        0xBA,           // CP D
        0xBB,           // CP E
        0xBC,           // CP H
        0xBD,           // CP L
        0xFE, 0x04,     // CP 0x04
    };
    init(0x0000, prog, sizeof(prog));
    T(7==step()); T(0x04 == _A);
    T(7==step()); T(0x05 == _B);
    T(7==step()); T(0x03 == _C);
    T(7==step()); T(0xff == _D);
    T(7==step()); T(0xaa == _E);
    T(7==step()); T(0x80 == _H);
    T(7==step()); T(0x7f == _L);
    T(4==step()); T(0x04 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_NF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_SF|Z80_VF|Z80_NF|Z80_CF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(7==step()); T(0x04 == _A); T(flags(Z80_ZF|Z80_NF));
}

/* AND A,r; AND A,n */
UTEST(z80, AND_A_rn) {
    uint8_t prog[] = {
        0x3E, 0xFF,             // LD A,0xFF
        0x06, 0x01,             // LD B,0x01
        0x0E, 0x03,             // LD C,0x02
        0x16, 0x04,             // LD D,0x04
        0x1E, 0x08,             // LD E,0x08
        0x26, 0x10,             // LD H,0x10
        0x2E, 0x20,             // LD L,0x20
        0xA0,                   // AND B
        0xF6, 0xFF,             // OR 0xFF
        0xA1,                   // AND C
        0xF6, 0xFF,             // OR 0xFF
        0xA2,                   // AND D
        0xF6, 0xFF,             // OR 0xFF
        0xA3,                   // AND E
        0xF6, 0xFF,             // OR 0xFF
        0xA4,                   // AND H
        0xF6, 0xFF,             // OR 0xFF
        0xA5,                   // AND L
        0xF6, 0xFF,             // OR 0xFF
        0xE6, 0x40,             // AND 0x40
        0xF6, 0xFF,             // OR 0xFF
        0xE6, 0xAA,             // AND 0xAA
    };
    init(0x0000, prog, sizeof(prog));
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(4==step()); T(0x01 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(4==step()); T(0x03 == _A); T(flags(Z80_HF|Z80_PF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(4==step()); T(0x04 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(4==step()); T(0x08 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(4==step()); T(0x10 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(4==step()); T(0x20 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(7==step()); T(0x40 == _A); T(flags(Z80_HF));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
    T(7==step()); T(0xAA == _A); T(flags(Z80_SF|Z80_HF|Z80_PF));
}

/* XOR A,r; XOR A,n */
UTEST(z80, XOR_A_rn) {
    uint8_t prog[] = {
        0x97,           // SUB A
        0x06, 0x01,     // LD B,0x01
        0x0E, 0x03,     // LD C,0x03
        0x16, 0x07,     // LD D,0x07
        0x1E, 0x0F,     // LD E,0x0F
        0x26, 0x1F,     // LD H,0x1F
        0x2E, 0x3F,     // LD L,0x3F
        0xAF,           // XOR A
        0xA8,           // XOR B
        0xA9,           // XOR C
        0xAA,           // XOR D
        0xAB,           // XOR E
        0xAC,           // XOR H
        0xAD,           // XOR L
        0xEE, 0x7F,     // XOR 0x7F
        0xEE, 0xFF,     // XOR 0xFF
    };
    init(0x0000, prog, sizeof(prog));
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_PF));
    T(4==step()); T(0x01 == _A); T(flags(0));
    T(4==step()); T(0x02 == _A); T(flags(0));
    T(4==step()); T(0x05 == _A); T(flags(Z80_PF));
    T(4==step()); T(0x0A == _A); T(flags(Z80_PF));
    T(4==step()); T(0x15 == _A); T(flags(0));
    T(4==step()); T(0x2A == _A); T(flags(0));
    T(7==step()); T(0x55 == _A); T(flags(Z80_PF));
    T(7==step()); T(0xAA == _A); T(flags(Z80_SF|Z80_PF));
}

/* OR A,r; OR A,n */
UTEST(z80, OR_A_rn) {
    uint8_t prog[] = {
        0x97,           // SUB A
        0x06, 0x01,     // LD B,0x01
        0x0E, 0x02,     // LD C,0x02
        0x16, 0x04,     // LD D,0x04
        0x1E, 0x08,     // LD E,0x08
        0x26, 0x10,     // LD H,0x10
        0x2E, 0x20,     // LD L,0x20
        0xB7,           // OR A
        0xB0,           // OR B
        0xB1,           // OR C
        0xB2,           // OR D
        0xB3,           // OR E
        0xB4,           // OR H
        0xB5,           // OR L
        0xF6, 0x40,     // OR 0x40
        0xF6, 0x80,     // OR 0x80
    };
    init(0x0000, prog, sizeof(prog));
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_PF));
    T(4==step()); T(0x01 == _A); T(flags(0));
    T(4==step()); T(0x03 == _A); T(flags(Z80_PF));
    T(4==step()); T(0x07 == _A); T(flags(0));
    T(4==step()); T(0x0F == _A); T(flags(Z80_PF));
    T(4==step()); T(0x1F == _A); T(flags(0));
    T(4==step()); T(0x3F == _A); T(flags(Z80_PF));
    T(7==step()); T(0x7F == _A); T(flags(0));
    T(7==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_PF));
}

UTEST_MAIN()



