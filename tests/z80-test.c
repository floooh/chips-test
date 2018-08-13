//------------------------------------------------------------------------------
//  z80-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80.h"
#include <stdio.h>

#define _A z80_a(&cpu)
#define _F z80_f(&cpu)
#define _L z80_l(&cpu)
#define _H z80_h(&cpu)
#define _E z80_e(&cpu)
#define _D z80_d(&cpu)
#define _C z80_c(&cpu)
#define _B z80_b(&cpu)
#define _FA z80_fa(&cpu)
#define _HL z80_hl(&cpu)
#define _DE z80_de(&cpu)
#define _BC z80_bc(&cpu)
#define _FA_ z80_fa_(&cpu)
#define _HL_ z80_hl_(&cpu)
#define _DE_ z80_de_(&cpu)
#define _BC_ z80_bc_(&cpu)
#define _PC z80_pc(&cpu)
#define _SP z80_sp(&cpu)
#define _WZ z80_wz(&cpu)
#define _IR z80_ir(&cpu)
#define _I z80_i(&cpu)
#define _R z80_r(&cpu)
#define _IX z80_ix(&cpu)
#define _IY z80_iy(&cpu)
#define _IM z80_im(&cpu)
#define _IFF1 z80_iff1(&cpu)
#define _IFF2 z80_iff2(&cpu)
#define _EI_PENDING z80_ei_pending(&cpu)

z80_t cpu;
uint8_t mem[1<<16] = { 0 };
uint16_t out_port = 0;
uint8_t out_byte = 0;
uint64_t tick(int num, uint64_t pins, void* user_data) {
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            /* memory read */
            Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
        }
        else if (pins & Z80_WR) {
            /* memory write */
            mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            /* IN */
            Z80_SET_DATA(pins, (Z80_GET_ADDR(pins) & 0xFF) * 2);
        }
        else if (pins & Z80_WR) {
            /* OUT */
            out_port = Z80_GET_ADDR(pins);
            out_byte = Z80_GET_DATA(pins);
        }
    }
    return pins;
}

void init() {
    z80_init(&cpu, &(z80_desc_t) { .tick_cb = tick, });
    z80_set_f(&cpu, 0);
    z80_set_fa_(&cpu, 0x00FF);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    uint32_t ticks = z80_exec(&cpu,0);
    while (!z80_opdone(&cpu)) {
        ticks += z80_exec(&cpu,0);
    }
    return ticks;
}

bool flags(uint8_t expected) {
    /* don't check undocumented flags */
    return (z80_f(&cpu) & ~(Z80_YF|Z80_XF)) == expected;
}

uint16_t mem16(uint16_t addr) {
    uint8_t l = mem[addr];
    uint8_t h = mem[addr+1];
    return (h<<8)|l;
}

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void SET_GET() {
    puts(">>> SET GET registers");
    init();
    z80_set_f(&cpu, 0x01); z80_set_a(&cpu, 0x23);
    z80_set_h(&cpu, 0x45); z80_set_l(&cpu, 0x67);
    z80_set_d(&cpu, 0x89); z80_set_e(&cpu, 0xAB);
    z80_set_b(&cpu, 0xCD); z80_set_c(&cpu, 0xEF);
    T(0x01 == _F); T(0x23 == _A); T(0x0123 == _FA);
    T(0x45 == _H); T(0x67 == _L); T(0x4567 == _HL);
    T(0x89 == _D); T(0xAB == _E); T(0x89AB == _DE);
    T(0xCD == _B); T(0xEF == _C); T(0xCDEF == _BC);

    z80_set_fa(&cpu, 0x1234);
    z80_set_hl(&cpu, 0x5678);
    z80_set_de(&cpu, 0x9abc);
    z80_set_bc(&cpu, 0xdef0);
    z80_set_fa_(&cpu, 0x4321);
    z80_set_hl_(&cpu, 0x8765);
    z80_set_de_(&cpu, 0xCBA9);
    z80_set_bc_(&cpu, 0x0FED);
    T(0x12 == _F); T(0x34 == _A); T(0x1234 == _FA);
    T(0x56 == _H); T(0x78 == _L); T(0x5678 == _HL);
    T(0x9A == _D); T(0xBC == _E); T(0x9ABC == _DE);
    T(0xDE == _B); T(0xF0 == _C); T(0xDEF0 == _BC);
    T(0x4321 == _FA_);
    T(0x8765 == _HL_);
    T(0xCBA9 == _DE_);
    T(0x0FED == _BC_);

    z80_set_pc(&cpu, 0xCEDF);
    z80_set_wz(&cpu, 0x1324);
    z80_set_sp(&cpu, 0x2435);
    z80_set_i(&cpu, 0x35);
    z80_set_r(&cpu, 0x46);
    z80_set_ix(&cpu, 0x4657);
    z80_set_iy(&cpu, 0x5768);
    T(0xCEDF == _PC);
    T(0x1324 == _WZ);
    T(0x2435 == _SP);
    T(0x35 == _I);
    T(0x46 == _R);
    T(0x4657 == _IX); 
    T(0x5768 == _IY);

    z80_set_im(&cpu, 2);
    z80_set_iff1(&cpu, true);
    z80_set_iff2(&cpu, false);
    z80_set_ei_pending(&cpu, true);
    T(2 == _IM);
    T(_IFF1);
    T(!_IFF2);
    T(_EI_PENDING);
    z80_set_iff1(&cpu, false);
    z80_set_iff2(&cpu, true);
    z80_set_ei_pending(&cpu, false);
    T(!_IFF1);
    T(_IFF2);
    T(!_EI_PENDING);
}

/* LD A,R; LD A,I */
void LD_A_RI() {
    puts(">>> LD A,R; LD A,I");
    uint8_t prog[] = {
        0xED, 0x57,         // LD A,I
        0x97,               // SUB A
        0xED, 0x5F,         // LD A,R
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    z80_set_iff1(&cpu, true);
    z80_set_iff2(&cpu, true);
    z80_set_r(&cpu, 0x34);
    z80_set_i(&cpu, 0x01);
    z80_set_f(&cpu, Z80_CF);
    T(9 == step()); T(0x01 == _A); T(flags(Z80_PF|Z80_CF));
    T(4 == step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(9 == step()); T(0x39 == _A); T(flags(Z80_PF));
}

/* LD I,A; LD R,A */
void LD_IR_A() {
    puts(">>> LD I,A; LD R,A");
    uint8_t prog[] = {
        0x3E, 0x45,     // LD A,0x45
        0xED, 0x47,     // LD I,A
        0xED, 0x4F,     // LD R,A
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(7==step()); T(0x45 == _A);
    T(9==step()); T(0x45 == _I);
    T(9==step()); T(0x45 == _R);
}

/* LD r,s; LD r,n */
void LD_r_sn() {
    puts(">>> LD r,s; LD r,n");
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
    init();
    copy(0x0000, prog, sizeof(prog));

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
void LD_r_iHLi() {
    puts(">>> LD r,(HL)");
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
    init();
    copy(0x0000, prog, sizeof(prog));

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

/* LD r,(IX|IY+d) */
void LD_r_iIXIYi() {
    puts(">>> LD r,(IX+d); LD r,(IY+d)");
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
    copy(0x0000, prog, sizeof(prog));
    init();

    T(14==step()); T(0x1003 == _IX);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]); T(0x1003 == _WZ);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]); T(0x1004 == _WZ);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]); T(0x1005 == _WZ);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]); T(0x1002 == _WZ);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]);
}

/* LD (HL),r */
void LD_iHLi_r() {
    puts(">>> LD (HL),r");
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
    init();
    copy(0x0000, prog, sizeof(prog));

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
void LD_iIXIYi_r() {
    puts(">>> LD (IX+d),r; LD (IY+d)");
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
    copy(0x0000, prog, sizeof(prog));
    init();

    T(14==step()); T(0x1003 == _IX);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x12 == _A);
    T(19==step()); T(0x12 == mem[0x1003]);
    T(7 ==step()); T(0x13 == _B);
    T(19==step()); T(0x13 == mem[0x1004]);
    T(7 ==step()); T(0x14 == _C);
    T(19==step()); T(0x14 == mem[0x1005]);
    T(7 ==step()); T(0x15 == _D);
    T(19==step()); T(0x15 == mem[0x1002]);
    T(7 ==step()); T(0x16 == _E);
    T(19==step()); T(0x16 == mem[0x1001]);
    T(7 ==step()); T(0x17 == _H);
    T(19==step()); T(0x17 == mem[0x1006]);
    T(7 ==step()); T(0x18 == _L);
    T(19==step()); T(0x18 == mem[0x1000]);
}

/* LD (HL),n */
void LD_iHLi_n() {
    puts(">>> LD (HL),n");
    uint8_t prog[] = {
        0x21, 0x00, 0x20,   // LD HL,0x2000
        0x36, 0x33,         // LD (HL),0x33
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x36, 0x65,         // LD (HL),0x65
    };
    init();
    copy(0x0000, prog, sizeof(prog));

    T(10==step()); T(0x2000 == _HL);
    T(10==step()); T(0x33 == mem[0x2000]);
    T(10==step()); T(0x1000 == _HL);
    T(10==step()); T(0x65 == mem[0x1000]);
}

/* LD (IX|IY+d),n */
void LD_iIXIYi_n() {
    puts(">>> LD (IX+d),n; LD (IY+d),n");
    uint8_t prog[] = {
        0xDD, 0x21, 0x00, 0x20,     // LD IX,0x2000
        0xDD, 0x36, 0x02, 0x33,     // LD (IX+2),0x33
        0xDD, 0x36, 0xFE, 0x11,     // LD (IX-2),0x11
        0xFD, 0x21, 0x00, 0x10,     // LD IY,0x1000
        0xFD, 0x36, 0x01, 0x22,     // LD (IY+1),0x22
        0xFD, 0x36, 0xFF, 0x44,     // LD (IY-1),0x44
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    T(14==step()); T(0x2000 == _IX);
    T(19==step()); T(0x33 == mem[0x2002]);
    T(19==step()); T(0x11 == mem[0x1FFE]);
    T(14==step()); T(0x1000 == _IY);
    T(19==step()); T(0x22 == mem[0x1001]);
    T(19==step()); T(0x44 == mem[0x0FFF]);
}

/* LD A,(BC); LD A,(DE); LD A,(nn) */
void LD_A_iBCDEnni() {
    puts(">>> LD A,(BC); LD A,(DE); LD A,(nn)");
    uint8_t data[] = {
        0x11, 0x22, 0x33
    };
    copy(0x1000, data, sizeof(data));
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x0A,               // LD A,(BC)
        0x1A,               // LD A,(DE)
        0x3A, 0x02, 0x10,   // LD A,(0x1002)
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    T(10==step()); T(0x1000 == _BC);
    T(10==step()); T(0x1001 == _DE);
    T(7 ==step()); T(0x11 == _A); T(0x1001 == _WZ);
    T(7 ==step()); T(0x22 == _A); T(0x1002 == _WZ);
    T(13==step()); T(0x33 == _A); T(0x1003 == _WZ);
}

/* LD (BC),A; LD (DE),A; LD (nn),A */
void LD_iBCDEnni_A() {
    puts(">>> LD (BC),A; LD (DE),A; LD (nn),A");
    uint8_t prog[] = {
        0x01, 0x00, 0x10,   // LD BC,0x1000
        0x11, 0x01, 0x10,   // LD DE,0x1001
        0x3E, 0x77,         // LD A,0x77
        0x02,               // LD (BC),A
        0x12,               // LD (DE),A
        0x32, 0x02, 0x10,   // LD (0x1002),A
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _BC);
    T(10==step()); T(0x1001 == _DE);
    T(7 ==step()); T(0x77 == _A);
    T(7 ==step()); T(0x77 == mem[0x1000]); T(0x7701 == _WZ);
    T(7 ==step()); T(0x77 == mem[0x1001]); T(0x7702 == _WZ);
    T(13==step()); T(0x77 == mem[0x1002]); T(0x7703 == _WZ);
}

/* LD dd,nn; LD IX,nn; LD IY,nn */
void LD_ddIXIY_nn() {
    puts(">>> LD dd,nn; LD IX,nn; LD IY,nn");
    uint8_t prog[] = {
        0x01, 0x34, 0x12,       // LD BC,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0x21, 0xBC, 0x9A,       // LD HL,0x9ABC
        0x31, 0x68, 0x13,       // LD SP,0x1368
        0xDD, 0x21, 0x21, 0x43, // LD IX,0x4321
        0xFD, 0x21, 0x65, 0x87, // LD IY,0x8765
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1234 == _BC);
    T(10==step()); T(0x5678 == _DE);
    T(10==step()); T(0x9ABC == _HL);
    T(10==step()); T(0x1368 == _SP);
    T(14==step()); T(0x4321 == _IX);
    T(14==step()); T(0x8765 == _IY);
}

/* LD HL,(nn); LD dd,(nn); LD IX,(nn); LD IY,(nn) */
void LD_HLddIXIY_inni() {
    puts(">>> LD dd,(nn); LD IX,(nn); LD IY,(nn)");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    };
    uint8_t prog[] = {
        0x2A, 0x00, 0x10,           // LD HL,(0x1000)
        0xED, 0x4B, 0x01, 0x10,     // LD BC,(0x1001)
        0xED, 0x5B, 0x02, 0x10,     // LD DE,(0x1002)
        0xED, 0x6B, 0x03, 0x10,     // LD HL,(0x1003) undocumented 'long' version
        0xED, 0x7B, 0x04, 0x10,     // LD SP,(0x1004)
        0xDD, 0x2A, 0x05, 0x10,     // LD IX,(0x1005)
        0xFD, 0x2A, 0x06, 0x10,     // LD IY,(0x1006)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(16==step()); T(0x0201 == _HL); T(0x1001 == _WZ);
    T(20==step()); T(0x0302 == _BC); T(0x1002 == _WZ);
    T(20==step()); T(0x0403 == _DE); T(0x1003 == _WZ);
    T(20==step()); T(0x0504 == _HL); T(0x1004 == _WZ);
    T(20==step()); T(0x0605 == _SP); T(0x1005 == _WZ);
    T(20==step()); T(0x0706 == _IX); T(0x1006 == _WZ);
    T(20==step()); T(0x0807 == _IY); T(0x1007 == _WZ);
}

/* LD (nn),HL; LD (nn),dd; LD (nn),IX; LD (nn),IY */
void LD_inni_HLddIXIY() {
    puts(">>> LD (nn),dd; LD (nn),IX; LD (nn),IY");
    uint8_t prog[] = {
        0x21, 0x01, 0x02,           // LD HL,0x0201
        0x22, 0x00, 0x10,           // LD (0x1000),HL
        0x01, 0x34, 0x12,           // LD BC,0x1234
        0xED, 0x43, 0x02, 0x10,     // LD (0x1002),BC
        0x11, 0x78, 0x56,           // LD DE,0x5678
        0xED, 0x53, 0x04, 0x10,     // LD (0x1004),DE
        0x21, 0xBC, 0x9A,           // LD HL,0x9ABC
        0xED, 0x63, 0x06, 0x10,     // LD (0x1006),HL undocumented 'long' version
        0x31, 0x68, 0x13,           // LD SP,0x1368
        0xED, 0x73, 0x08, 0x10,     // LD (0x1008),SP
        0xDD, 0x21, 0x21, 0x43,     // LD IX,0x4321
        0xDD, 0x22, 0x0A, 0x10,     // LD (0x100A),IX
        0xFD, 0x21, 0x65, 0x87,     // LD IY,0x8765
        0xFD, 0x22, 0x0C, 0x10,     // LD (0x100C),IY
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x0201 == _HL);
    T(16==step()); T(0x0201 == mem16(0x1000)); T(0x1001 == _WZ);
    T(10==step()); T(0x1234 == _BC);
    T(20==step()); T(0x1234 == mem16(0x1002)); T(0x1003 == _WZ);
    T(10==step()); T(0x5678 == _DE);
    T(20==step()); T(0x5678 == mem16(0x1004)); T(0x1005 == _WZ);
    T(10==step()); T(0x9ABC == _HL);
    T(20==step()); T(0x9ABC == mem16(0x1006)); T(0x1007 == _WZ);
    T(10==step()); T(0x1368 == _SP);
    T(20==step()); T(0x1368 == mem16(0x1008)); T(0x1009 == _WZ);
    T(14==step()); T(0x4321 == _IX);
    T(20==step()); T(0x4321 == mem16(0x100A)); T(0x100B == _WZ);
    T(14==step()); T(0x8765 == _IY);
    T(20==step()); T(0x8765 == mem16(0x100C)); T(0x100D == _WZ);
}

/* LD SP,HL; LD SP,IX; LD SP,IY */
void LD_SP_HLIXIY() {
    puts(">>> LD SP,HL; LD SP,IX; LD SP,IY");
    uint8_t prog[] = {
        0x21, 0x34, 0x12,           // LD HL,0x1234
        0xDD, 0x21, 0x78, 0x56,     // LD IX,0x5678
        0xFD, 0x21, 0xBC, 0x9A,     // LD IY,0x9ABC
        0xF9,                       // LD SP,HL
        0xDD, 0xF9,                 // LD SP,IX
        0xFD, 0xF9,                 // LD SP,IY
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1234 == _HL);
    T(14==step()); T(0x5678 == _IX);
    T(14==step()); T(0x9ABC == _IY);
    T(6 ==step()); T(0x1234 == _SP);
    T(10==step()); T(0x5678 == _SP);
    T(10==step()); T(0x9ABC == _SP);
}

/* PUSH qq; PUSH IX; PUSH IY; POP qq; POP IX; POP IY */
void PUSH_POP_qqIXIY() {
    puts(">>> PUSH qq; PUSH IX; PUSH IY; POP qq; POP IX; POP IY");
    uint8_t prog[] = {
        0x01, 0x34, 0x12,       // LD BC,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0x21, 0xBC, 0x9A,       // LD HL,0x9ABC
        0x3E, 0xEF,             // LD A,0xEF
        0xDD, 0x21, 0x45, 0x23, // LD IX,0x2345
        0xFD, 0x21, 0x89, 0x67, // LD IY,0x6789
        0x31, 0x00, 0x01,       // LD SP,0x0100
        0xF5,                   // PUSH AF
        0xC5,                   // PUSH BC
        0xD5,                   // PUSH DE
        0xE5,                   // PUSH HL
        0xDD, 0xE5,             // PUSH IX
        0xFD, 0xE5,             // PUSH IY
        0xF1,                   // POP AF
        0xC1,                   // POP BC
        0xD1,                   // POP DE
        0xE1,                   // POP HL
        0xDD, 0xE1,             // POP IX
        0xFD, 0xE1,             // POP IY
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1234 == _BC);
    T(10==step()); T(0x5678 == _DE);
    T(10==step()); T(0x9ABC == _HL);
    T(7 ==step()); T(0x00EF == _FA);
    T(14==step()); T(0x2345 == _IX);
    T(14==step()); T(0x6789 == _IY);
    T(10==step()); T(0x0100 == _SP);
    T(11==step()); T(0xEF00 == mem16(0x00FE)); T(0x00FE == _SP);
    T(11==step()); T(0x1234 == mem16(0x00FC)); T(0x00FC == _SP);
    T(11==step()); T(0x5678 == mem16(0x00FA)); T(0x00FA == _SP);
    T(11==step()); T(0x9ABC == mem16(0x00F8)); T(0x00F8 == _SP);
    T(15==step()); T(0x2345 == mem16(0x00F6)); T(0x00F6 == _SP);
    T(15==step()); T(0x6789 == mem16(0x00F4)); T(0x00F4 == _SP);
    T(10==step()); T(0x8967 == _FA); T(0x00F6 == _SP);
    T(10==step()); T(0x2345 == _BC); T(0x00F8 == _SP);
    T(10==step()); T(0x9ABC == _DE); T(0x00FA == _SP);
    T(10==step()); T(0x5678 == _HL); T(0x00FC == _SP);
    T(14==step()); T(0x1234 == _IX); T(0x00FE == _SP);
    T(14==step()); T(0xEF00 == _IY); T(0x0100 == _SP);
}

/* EX DE,HL; EX AF,AF'; EX (SP),HL; EX (SP),IX; EX (SP),IY */
void EX() {
    puts(">>> EX DE,HL; EX AF,AF'; EX (SP),HL; EX (SP),IX; EX (SP),IY");
    uint8_t prog[] = {
        0x21, 0x34, 0x12,       // LD HL,0x1234
        0x11, 0x78, 0x56,       // LD DE,0x5678
        0xEB,                   // EX DE,HL
        0x3E, 0x11,             // LD A,0x11
        0x08,                   // EX AF,AF'
        0x3E, 0x22,             // LD A,0x22
        0x08,                   // EX AF,AF'
        0x01, 0xBC, 0x9A,       // LD BC,0x9ABC
        0xD9,                   // EXX
        0x21, 0x11, 0x11,       // LD HL,0x1111
        0x11, 0x22, 0x22,       // LD DE,0x2222
        0x01, 0x33, 0x33,       // LD BC,0x3333
        0xD9,                   // EXX
        0x31, 0x00, 0x01,       // LD SP,0x0100
        0xD5,                   // PUSH DE
        0xE3,                   // EX (SP),HL
        0xDD, 0x21, 0x99, 0x88, // LD IX,0x8899
        0xDD, 0xE3,             // EX (SP),IX
        0xFD, 0x21, 0x77, 0x66, // LD IY,0x6677
        0xFD, 0xE3,             // EX (SP),IY
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1234 == _HL);
    T(10==step()); T(0x5678 == _DE);
    T(4 ==step()); T(0x1234 == _DE); T(0x5678 == _HL);
    T(7 ==step()); T(0x0011 == _FA); T(0x00FF == _FA_);
    T(4 ==step()); T(0x00FF == _FA); T(0x0011 == _FA_);
    T(7 ==step()); T(0x0022 == _FA); T(0x0011 == _FA_);
    T(4 ==step()); T(0x0011 == _FA); T(0x0022 == _FA_);
    T(10==step()); T(0x9ABC == _BC);
    T(4 ==step());
    T(0xFFFF == _HL); T(0x5678 == _HL_);
    T(0xFFFF == _DE); T(0x1234 == _DE_);
    T(0xFFFF == _BC); T(0x9ABC == _BC_);
    T(10==step()); T(0x1111 == _HL);
    T(10==step()); T(0x2222 == _DE);
    T(10==step()); T(0x3333 == _BC);
    T(4 ==step());
    T(0x5678 == _HL); T(0x1111 == _HL_);
    T(0x1234 == _DE); T(0x2222 == _DE_);
    T(0x9ABC == _BC); T(0x3333 == _BC_);
    T(10==step()); T(0x0100 == _SP);
    T(11==step()); T(0x1234 == mem16(0x00FE));
    T(19==step()); T(0x1234 == _HL); T(_WZ == _HL); T(0x5678 == mem16(0x00FE));
    T(14==step()); T(0x8899 == _IX);
    T(23==step()); T(0x5678 == _IX); T(_WZ == _IX); T(0x8899 == mem16(0x00FE));
    T(14==step()); T(0x6677 == _IY);
    T(23==step()); T(0x8899 == _IY); T(_WZ == _IY); T(0x6677 == mem16(0x00FE));
}

/* ADD A,r; ADD A,n */
void ADD_A_rn() {
    puts(">>> ADD A,r; ADD A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

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

/* ADD A,(HL); ADD A,(IX+d); ADD A,(IY+d) */
void ADD_A_iHLIXIYi() {
    puts(">>> ADD A,(HL); ADD A,(IX+d); ADD A,(IY+d)");
    uint8_t data[] = { 0x41, 0x61, 0x81 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x86,                   // ADD A,(HL)
        0xDD, 0x86, 0x01,       // ADD A,(IX+1)
        0xFD, 0x86, 0xFF,       // ADD A,(IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _HL);
    T(14==step()); T(0x1000 == _IX);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x00 == _A);
    T(7 ==step()); T(0x41 == _A); T(flags(0));
    T(19==step()); T(0xA2 == _A); T(flags(Z80_SF|Z80_VF));
    T(19==step()); T(0x23 == _A); T(flags(Z80_VF|Z80_CF));
}

/* ADC A,r; ADC A,n */
void ADC_A_rn() {
    puts(">>> ADC A,r; ADD A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

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

/* ADC A,(HL); ADC A,(IX+d); ADC A,(IY+d) */
void ADC_A_iHLIXIYi() {
    puts(">>> ADC A,(HL); ADC A,(IX+d); ADC A,(IY+d)");
    uint8_t data[] = { 0x41, 0x61, 0x81, 0x2 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x86,                   // ADD A,(HL)
        0xDD, 0x8E, 0x01,       // ADC A,(IX+1)
        0xFD, 0x8E, 0xFF,       // ADC A,(IY-1)
        0xDD, 0x8E, 0x03,       // ADC A,(IX+3)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _HL);
    T(14==step()); T(0x1000 == _IX);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x00 == _A);
    T(7 ==step()); T(0x41 == _A); T(flags(0));
    T(19==step()); T(0xA2 == _A); T(flags(Z80_SF|Z80_VF));
    T(19==step()); T(0x23 == _A); T(flags(Z80_VF|Z80_CF));
    T(19==step()); T(0x26 == _A); T(flags(0));
}

/* SUB A,r; SUB A,n */
void SUB_A_rn() {
    puts(">>> SUB A,r; SUB A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();
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

/* SUB A,(HL); SUB A,(IX+d); SUB A,(IY+d) */
void SUB_A_iHLIXIYi() {
    puts(">>> SUB A,(HL); SUB A,(IX+d); SUB A,(IY+d)");
    uint8_t data[] = { 0x41, 0x61, 0x81 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x96,                   // SUB A,(HL)
        0xDD, 0x96, 0x01,       // SUB A,(IX+1)
        0xFD, 0x96, 0xFE,       // SUB A,(IY-2)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _HL);
    T(14==step()); T(0x1000 == _IX);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x00 == _A);
    T(7 ==step()); T(0xBF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(19==step()); T(0x5E == _A); T(flags(Z80_VF|Z80_NF));
    T(19==step()); T(0xFD == _A); T(flags(Z80_SF|Z80_NF|Z80_CF));
}

/* SBC A,r; SBC A,n */
void SBC_A_rn() {
    puts(">>> SBC A,r; SBC A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

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

/* SBC A,(HL); SBC A,(IX+d); SBC A,(IY+d) */
void SBC_A_iHLIXIYi() {
    puts(">>> SBC A,(HL); SBC A,(IX+d); SBC A,(IY+d)");
    uint8_t data[] = { 0x41, 0x61, 0x81 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x00,             // LD A,0x00
        0x9E,                   // SBC A,(HL)
        0xDD, 0x9E, 0x01,       // SBC A,(IX+1)
        0xFD, 0x9E, 0xFE,       // SBC A,(IY-2)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _HL);
    T(14==step()); T(0x1000 == _IX);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x00 == _A);
    T(7 ==step()); T(0xBF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(19==step()); T(0x5D == _A); T(flags(Z80_VF|Z80_NF));
    T(19==step()); T(0xFC == _A); T(flags(Z80_SF|Z80_NF|Z80_CF));
}

/* CP A,r; CP A,n */
void CP_A_rn() {
    puts(">>> CP A,r; CP A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

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

/* CP A,(HL); CP A,(IX+d); CP A,(IY+d) */
void CP_A_iHLIXIYi() {
    puts(">>> CP A,(HL); CP A,(IX+d); CP A,(IY+d)");
    uint8_t data[] = { 0x41, 0x61, 0x22 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10, // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10, // LD IY,0x1003
        0x3E, 0x41,             // LD A,0x41
        0xBE,                   // CP (HL)
        0xDD, 0xBE, 0x01,       // CP (IX+1)
        0xFD, 0xBE, 0xFF,       // CP (IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x1000 == _HL);
    T(14==step()); T(0x1000 == _IX);
    T(14==step()); T(0x1003 == _IY);
    T(7 ==step()); T(0x41 == _A);
    T(7 ==step()); T(0x41 == _A); T(flags(Z80_ZF|Z80_NF));
    T(19==step()); T(0x41 == _A); T(flags(Z80_SF|Z80_NF|Z80_CF));
    T(19==step()); T(0x41 == _A); T(flags(Z80_HF|Z80_NF));
}

/* AND A,r; AND A,n */
void AND_A_rn() {
    puts(">>> AND A,r; AND A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
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

/* AND A,(HL); AND A,(IX+d); AND A,(IY+d) */
void AND_A_iHLIXIYi() {
    puts(">>> AND A,(HL); AND A,(IX+d); AND A,(IY+d)");
    uint8_t data[] = { 0xFE, 0xAA, 0x99 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x3E, 0xFF,                 // LD A,0xFF
        0xA6,                       // AND (HL)
        0xDD, 0xA6, 0x01,           // AND (IX+1)
        0xFD, 0xA6, 0xFF,           // AND (IX-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    step(); step(); step(); step();
    T(7 ==step()); T(0xFE == _A); T(flags(Z80_SF|Z80_HF));
    T(19==step()); T(0xAA == _A); T(flags(Z80_SF|Z80_HF|Z80_PF));
    T(19==step()); T(0x88 == _A); T(flags(Z80_SF|Z80_HF|Z80_PF));
}

/* XOR A,r; XOR A,n */
void XOR_A_rn() {
    puts(">>> XOR A,r; XOR A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
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
void OR_A_rn() {
    puts(">>> OR A,r; OR A,n");
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
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
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

/* OR A,(HL); OR A,(IX+d); OR A,(IY+d) */
/* XOR A,(HL); XOR A,(IX+d); XOR A,(IY+d) */
void OR_XOR_A_iHLIXIYi() {
    puts(">>> OR/XOR A,(HL); OR/XOR A,(IX+d); OR/XOR A,(IY+d)");
    uint8_t data[] = { 0x41, 0x62, 0x84 };
    uint8_t prog[] = {
        0x3E, 0x00,                 // LD A,0x00
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xB6,                       // OR (HL)
        0xDD, 0xB6, 0x01,           // OR (IX+1)
        0xFD, 0xB6, 0xFF,           // OR (IY-1)
        0xAE,                       // XOR (HL)
        0xDD, 0xAE, 0x01,           // XOR (IX+1)
        0xFD, 0xAE, 0xFF,           // XOR (IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    step(); step(); step(); step();
    T(7 ==step()); T(0x41 == _A); T(flags(Z80_PF));
    T(19==step()); T(0x63 == _A); T(flags(Z80_PF));
    T(19==step()); T(0xE7 == _A); T(flags(Z80_SF|Z80_PF));
    T(7 ==step()); T(0xA6 == _A); T(flags(Z80_SF|Z80_PF));
    T(19==step()); T(0xC4 == _A); T(flags(Z80_SF));
    T(19==step()); T(0x40 == _A); T(flags(0));
}

/* INC r; DEC r */
void INC_DEC_r() {
    puts(">>> INC r; DEC r");
    uint8_t prog[] = {
        0x3e, 0x00,         // LD A,0x00
        0x06, 0xFF,         // LD B,0xFF
        0x0e, 0x0F,         // LD C,0x0F
        0x16, 0x0E,         // LD D,0x0E
        0x1E, 0x7F,         // LD E,0x7F
        0x26, 0x3E,         // LD H,0x3E
        0x2E, 0x23,         // LD L,0x23
        0x3C,               // INC A
        0x3D,               // DEC A
        0x04,               // INC B
        0x05,               // DEC B
        0x0C,               // INC C
        0x0D,               // DEC C
        0x14,               // INC D
        0x15,               // DEC D
        0xFE, 0x01,         // CP 0x01  // set carry flag (should be preserved)
        0x1C,               // INC E
        0x1D,               // DEC E
        0x24,               // INC H
        0x25,               // DEC H
        0x2C,               // INC L
        0x2D,               // DEC L
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(0x00 == _A);
    T(0xFF == _B);
    T(0x0F == _C);
    T(0x0E == _D);
    T(0x7F == _E);
    T(0x3E == _H);
    T(0x23 == _L);
    T(4==step()); T(0x01 == _A); T(flags(0));
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0x00 == _B); T(flags(Z80_ZF|Z80_HF));
    T(4==step()); T(0xFF == _B); T(flags(Z80_SF|Z80_HF|Z80_NF));
    T(4==step()); T(0x10 == _C); T(flags(Z80_HF));
    T(4==step()); T(0x0F == _C); T(flags(Z80_HF|Z80_NF));
    T(4==step()); T(0x0F == _D); T(flags(0));
    T(4==step()); T(0x0E == _D); T(flags(Z80_NF));
    T(7==step()); T(0x00 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x80 == _E); T(flags(Z80_SF|Z80_HF|Z80_VF|Z80_CF));
    T(4==step()); T(0x7F == _E); T(flags(Z80_HF|Z80_VF|Z80_NF|Z80_CF));
    T(4==step()); T(0x3F == _H); T(flags(Z80_CF));
    T(4==step()); T(0x3E == _H); T(flags(Z80_NF|Z80_CF));
    T(4==step()); T(0x24 == _L); T(flags(Z80_CF));
    T(4==step()); T(0x23 == _L); T(flags(Z80_NF|Z80_CF));
}

/* INC (HL); INC (IX+d); INC (IY+d); DEC (HL); DEC (IX+d); DEC (IY+d) */
void INC_DEC_iHLIXIYi() {
    puts(">>> INC/DEC (HL)/(IX+d)/(IY+d)");
    uint8_t data[] = { 0x00, 0x3F, 0x7F };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1000
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0x35,                       // DEC (HL)
        0x34,                       // INC (HL)
        0xDD, 0x34, 0x01,           // INC (IX+1)
        0xDD, 0x35, 0x01,           // DEC (IX+1)
        0xFD, 0x34, 0xFF,           // INC (IY-1)
        0xFD, 0x35, 0xFF,           // DEC (IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(11==step()); T(0xFF == mem[0x1000]); T(flags(Z80_SF|Z80_HF|Z80_NF));
    T(11==step()); T(0x00 == mem[0x1000]); T(flags(Z80_ZF|Z80_HF));
    T(23==step()); T(0x40 == mem[0x1001]); T(flags(Z80_HF));
    T(23==step()); T(0x3F == mem[0x1001]); T(flags(Z80_HF|Z80_NF));
    T(23==step()); T(0x80 == mem[0x1002]); T(flags(Z80_SF|Z80_HF|Z80_VF));
    T(23==step()); T(0x7F == mem[0x1002]); T(flags(Z80_HF|Z80_PF|Z80_NF));
}

/* INC ss; INC IX; INC IY; DEC ss; DEC IX; DEC IY */
void INC_DEC_ssIXIY() {
    puts(">>> INC/DEC ss/IX/IY");
    uint8_t prog[] = {
        0x01, 0x00, 0x00,       // LD BC,0x0000
        0x11, 0xFF, 0xFF,       // LD DE,0xffff
        0x21, 0xFF, 0x00,       // LD HL,0x00ff
        0x31, 0x11, 0x11,       // LD SP,0x1111
        0xDD, 0x21, 0xFF, 0x0F, // LD IX,0x0fff
        0xFD, 0x21, 0x34, 0x12, // LD IY,0x1234
        0x0B,                   // DEC BC
        0x03,                   // INC BC
        0x13,                   // INC DE
        0x1B,                   // DEC DE
        0x23,                   // INC HL
        0x2B,                   // DEC HL
        0x33,                   // INC SP
        0x3B,                   // DEC SP
        0xDD, 0x23,             // INC IX
        0xDD, 0x2B,             // DEC IX
        0xFD, 0x23,             // INC IX
        0xFD, 0x2B,             // DEC IX
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    for (int i = 0; i < 6; i++) {
        step();
    }
    T(6 ==step()); T(0xFFFF == _BC);
    T(6 ==step()); T(0x0000 == _BC);
    T(6 ==step()); T(0x0000 == _DE);
    T(6 ==step()); T(0xFFFF == _DE);
    T(6 ==step()); T(0x0100 == _HL);
    T(6 ==step()); T(0x00FF == _HL);
    T(6 ==step()); T(0x1112 == _SP);
    T(6 ==step()); T(0x1111 == _SP);
    T(10==step()); T(0x1000 == _IX);
    T(10==step()); T(0x0FFF == _IX);
    T(10==step()); T(0x1235 == _IY);
    T(10==step()); T(0x1234 == _IY);
}

/* RLCA; RLA; RRCA; RRA */
void RLCA_RLA_RRCA_RRA() {
    puts(">>> RLCA; RLA; RRCA; RRA");
    uint8_t prog[] = {
        0x3E, 0xA0,     // LD A,0xA0
        0x07,           // RLCA
        0x07,           // RLCA
        0x0F,           // RRCA
        0x0F,           // RRCA
        0x17,           // RLA
        0x17,           // RLA
        0x1F,           // RRA
        0x1F,           // RRA
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    z80_set_f(&cpu, 0xff);
    T(7==step()); T(0xA0 == _A);
    T(4==step()); T(0x41 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF|Z80_CF));
    T(4==step()); T(0x82 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF));
    T(4==step()); T(0x41 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF));
    T(4==step()); T(0xA0 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF|Z80_CF));
    T(4==step()); T(0x41 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF|Z80_CF));
    T(4==step()); T(0x83 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF));
    T(4==step()); T(0x41 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF|Z80_CF));
    T(4==step()); T(0xA0 == _A); T(flags(Z80_SF|Z80_ZF|Z80_VF|Z80_CF));
}

/* RLC r; RL r; RRC r; RR r */
void RLC_RL_RRC_RR_r() {
    puts(">>> RLC r; RL r; RRC r; RR r");
    uint8_t prog[] = {
        0x3E, 0x01,     // LD A,0x01
        0x06, 0xFF,     // LD B,0xFF
        0x0E, 0x03,     // LD C,0x03
        0x16, 0xFE,     // LD D,0xFE
        0x1E, 0x11,     // LD E,0x11
        0x26, 0x3F,     // LD H,0x3F
        0x2E, 0x70,     // LD L,0x70

        0xCB, 0x0F,     // RRC A
        0xCB, 0x07,     // RLC A
        0xCB, 0x08,     // RRC B
        0xCB, 0x00,     // RLC B
        0xCB, 0x01,     // RLC C
        0xCB, 0x09,     // RRC C
        0xCB, 0x02,     // RLC D
        0xCB, 0x0A,     // RRC D
        0xCB, 0x0B,     // RRC E
        0xCB, 0x03,     // RLC E
        0xCB, 0x04,     // RLC H
        0xCB, 0x0C,     // RCC H
        0xCB, 0x05,     // RLC L
        0xCB, 0x0D,     // RRC L

        0xCB, 0x1F,     // RR A
        0xCB, 0x17,     // RL A
        0xCB, 0x18,     // RR B
        0xCB, 0x10,     // RL B
        0xCB, 0x11,     // RL C
        0xCB, 0x19,     // RR C
        0xCB, 0x12,     // RL D
        0xCB, 0x1A,     // RR D
        0xCB, 0x1B,     // RR E
        0xCB, 0x13,     // RL E
        0xCB, 0x14,     // RL H
        0xCB, 0x1C,     // RR H
        0xCB, 0x15,     // RL L
        0xCB, 0x1D,     // RR L
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(8==step()); T(0x80 == _A); T(flags(Z80_SF|Z80_CF));
    T(8==step()); T(0x01 == _A); T(flags(Z80_CF));
    T(8==step()); T(0xFF == _B); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(8==step()); T(0xFF == _B); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(8==step()); T(0x06 == _C); T(flags(Z80_PF));
    T(8==step()); T(0x03 == _C); T(flags(Z80_PF));
    T(8==step()); T(0xFD == _D); T(flags(Z80_SF|Z80_CF));
    T(8==step()); T(0xFE == _D); T(flags(Z80_SF|Z80_CF));
    T(8==step()); T(0x88 == _E); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(8==step()); T(0x11 == _E); T(flags(Z80_PF|Z80_CF));
    T(8==step()); T(0x7E == _H); T(flags(Z80_PF));
    T(8==step()); T(0x3F == _H); T(flags(Z80_PF));
    T(8==step()); T(0xE0 == _L); T(flags(Z80_SF));
    T(8==step()); T(0x70 == _L); T(flags(0));
    T(8==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(8==step()); T(0x01 == _A); T(flags(0));
    T(8==step()); T(0x7F == _B); T(flags(Z80_CF));
    T(8==step()); T(0xFF == _B); T(flags(Z80_SF|Z80_PF));
    T(8==step()); T(0x06 == _C); T(flags(Z80_PF));
    T(8==step()); T(0x03 == _C); T(flags(Z80_PF));
    T(8==step()); T(0xFC == _D); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(8==step()); T(0xFE == _D); T(flags(Z80_SF));
    T(8==step()); T(0x08 == _E); T(flags(Z80_CF));
    T(8==step()); T(0x11 == _E); T(flags(Z80_PF));
    T(8==step()); T(0x7E == _H); T(flags(Z80_PF));
    T(8==step()); T(0x3F == _H); T(flags(Z80_PF));
    T(8==step()); T(0xE0 == _L); T(flags(Z80_SF));
    T(8==step()); T(0x70 == _L); T(flags(0));
}

/* RRC/RLC/RR/RL (HL)/(IX+d)/(IY+d) */
void RRC_RLC_RR_RL_iHLIXIYi() {
    puts(">>> RLC/RL/RRC/RR (HL)/(IX+d)/(IY+d)");
    uint8_t data[] = { 0x01, 0xFF, 0x11 };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1001
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xCB, 0x0E,                 // RRC (HL)
        0x7E,                       // LD A,(HL)
        0xCB, 0x06,                 // RLC (HL)
        0x7E,                       // LD A,(HL)
        0xDD, 0xCB, 0x01, 0x0E,     // RRC (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xDD, 0xCB, 0x01, 0x06,     // RLC (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xFD, 0xCB, 0xFF, 0x0E,     // RRC (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
        0xFD, 0xCB, 0xFF, 0x06,     // RLC (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
        0xCB, 0x1E,                 // RR (HL)
        0x7E,                       // LD A,(HL)
        0xCB, 0x16,                 // RL (HL)
        0x7E,                       // LD A,(HL)
        0xDD, 0xCB, 0x01, 0x1E,     // RR (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xDD, 0xCB, 0x01, 0x16,     // RL (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xFD, 0xCB, 0xFF, 0x16,     // RL (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
        0xFD, 0xCB, 0xFF, 0x1E,     // RR (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(15==step()); T(0x80 == mem[0x1000]); T(flags(Z80_SF|Z80_CF));
    T(7 ==step()); T(0x80 == _A);
    T(15==step()); T(0x01 == mem[0x1000]); T(flags(Z80_CF));
    T(7 ==step()); T(0x01 == _A);
    T(23==step()); T(0xFF == mem[0x1001]); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(19==step()); T(0xFF == _A);
    T(23==step()); T(0xFF == mem[0x1001]); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(19==step()); T(0xFF == _A);
    T(23==step()); T(0x88 == mem[0x1002]); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(19==step()); T(0x88 == _A);
    T(23==step()); T(0x11 == mem[0x1002]); T(flags(Z80_PF|Z80_CF));
    T(19==step()); T(0x11 == _A);
    T(15==step()); T(0x80 == mem[0x1000]); T(flags(Z80_SF|Z80_CF));
    T(7 ==step()); T(0x80 == _A);
    T(15==step()); T(0x01 == mem[0x1000]); T(flags(Z80_CF));
    T(7 ==step()); T(0x01 == _A);
    T(23==step()); T(0xFF == mem[0x1001]); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(19==step()); T(0xFF == _A);
    T(23==step()); T(0xFF == mem[0x1001]); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(19==step()); T(0xFF == _A);
    T(23==step()); T(0x23 == mem[0x1002]); T(flags(0));
    T(19==step()); T(0x23 == _A);
    T(23==step()); T(0x11 == mem[0x1002]); T(flags(Z80_PF|Z80_CF));
    T(19==step()); T(0x11 == _A);
}

/* SLA r */
void SLA_r() {
    puts(">>> SLA r");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0x06, 0x80,         // LD B,0x80
        0x0E, 0xAA,         // LD C,0xAA
        0x16, 0xFE,         // LD D,0xFE
        0x1E, 0x7F,         // LD E,0x7F
        0x26, 0x11,         // LD H,0x11
        0x2E, 0x00,         // LD L,0x00
        0xCB, 0x27,         // SLA A
        0xCB, 0x20,         // SLA B
        0xCB, 0x21,         // SLA C
        0xCB, 0x22,         // SLA D
        0xCB, 0x23,         // SLA E
        0xCB, 0x24,         // SLA H
        0xCB, 0x25,         // SLA L
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(8==step()); T(0x02 == _A); T(flags(0));
    T(8==step()); T(0x00 == _B); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(8==step()); T(0x54 == _C); T(flags(Z80_CF));
    T(8==step()); T(0xFC == _D); T(flags(Z80_SF|Z80_PF|Z80_CF));
    T(8==step()); T(0xFE == _E); T(flags(Z80_SF));
    T(8==step()); T(0x22 == _H); T(flags(Z80_PF));
    T(8==step()); T(0x00 == _L); T(flags(Z80_ZF|Z80_PF));
}

/* SLA (HL)/(IX+d)/(IY+d) */
void SLA_iHLIXIYi() {
    puts(">>> SLA (HL)/(IX+d)/(IY+d)");
    uint8_t data[] = { 0x01, 0x80, 0xAA };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1001
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xCB, 0x26,                 // SLA (HL)
        0x7E,                       // LD A,(HL)
        0xDD, 0xCB, 0x01, 0x26,     // SLA (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xFD, 0xCB, 0xFF, 0x26,     // SLA (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x000, prog, sizeof(prog));
    init();
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(15==step()); T(0x02 == mem[0x1000]); T(flags(0));
    T(7 ==step()); T(0x02 == _A);
    T(23==step()); T(0x00 == mem[0x1001]); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(19==step()); T(0x00 == _A);
    T(23==step()); T(0x54 == mem[0x1002]); T(flags(Z80_CF));
    T(19==step()); T(0x54 == _A);
}

/* SRA r */
void SRA_r() {
    puts(">>> SRA r");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0x06, 0x80,         // LD B,0x80
        0x0E, 0xAA,         // LD C,0xAA
        0x16, 0xFE,         // LD D,0xFE
        0x1E, 0x7F,         // LD E,0x7F
        0x26, 0x11,         // LD H,0x11
        0x2E, 0x00,         // LD L,0x00
        0xCB, 0x2F,         // SRA A
        0xCB, 0x28,         // SRA B
        0xCB, 0x29,         // SRA C
        0xCB, 0x2A,         // SRA D
        0xCB, 0x2B,         // SRA E
        0xCB, 0x2C,         // SRA H
        0xCB, 0x2D,         // SRA L
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    // skip loads
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(8==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(8==step()); T(0xC0 == _B); T(flags(Z80_SF|Z80_PF));
    T(8==step()); T(0xD5 == _C); T(flags(Z80_SF));
    T(8==step()); T(0xFF == _D); T(flags(Z80_SF|Z80_PF));
    T(8==step()); T(0x3F == _E); T(flags(Z80_PF|Z80_CF));
    T(8==step()); T(0x08 == _H); T(flags(Z80_CF));
    T(8==step()); T(0x00 == _L); T(flags(Z80_ZF|Z80_PF));
}

/* SRA (HL)/(IX+d)/(IY+d) */
void SRA_iHLIXIYi() {
    puts(">>> SRA (HL)/(IX+d)/(IY+d)");
    uint8_t data[] = { 0x01, 0x80, 0xAA };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1001
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xCB, 0x2E,                 // SRA (HL)
        0x7E,                       // LD A,(HL)
        0xDD, 0xCB, 0x01, 0x2E,     // SRA (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xFD, 0xCB, 0xFF, 0x2E,     // SRA (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x000, prog, sizeof(prog));
    init();
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(15==step()); T(0x00 == mem[0x1000]); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(7 ==step()); T(0x00 == _A);
    T(23==step()); T(0xC0 == mem[0x1001]); T(flags(Z80_SF|Z80_PF));
    T(19==step()); T(0xC0 == _A);
    T(23==step()); T(0xD5 == mem[0x1002]); T(flags(Z80_SF));
    T(19==step()); T(0xD5 == _A);
}

/* SRL r */
void SRL_r() {
    puts(">>> SRL r");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0x06, 0x80,         // LD B,0x80
        0x0E, 0xAA,         // LD C,0xAA
        0x16, 0xFE,         // LD D,0xFE
        0x1E, 0x7F,         // LD E,0x7F
        0x26, 0x11,         // LD H,0x11
        0x2E, 0x00,         // LD L,0x00
        0xCB, 0x3F,         // SRL A
        0xCB, 0x38,         // SRL B
        0xCB, 0x39,         // SRL C
        0xCB, 0x3A,         // SRL D
        0xCB, 0x3B,         // SRL E
        0xCB, 0x3C,         // SRL H
        0xCB, 0x3D,         // SRL L
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 7; i++) {
        step();
    }
    T(8==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(8==step()); T(0x40 == _B); T(flags(0));
    T(8==step()); T(0x55 == _C); T(flags(Z80_PF));
    T(8==step()); T(0x7F == _D); T(flags(0));
    T(8==step()); T(0x3F == _E); T(flags(Z80_PF|Z80_CF));
    T(8==step()); T(0x08 == _H); T(flags(Z80_CF));
    T(8==step()); T(0x00 == _L); T(flags(Z80_ZF|Z80_PF))
}

/* SRL (HL)/(IX+d)/(IY+d) */
void SRL_iHLIXIYi() {
    puts(">>> SRL (HL)/(IX+d)/(IY+d)");
    uint8_t data[] = { 0x01, 0x80, 0xAA };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,           // LD HL,0x1000
        0xDD, 0x21, 0x00, 0x10,     // LD IX,0x1001
        0xFD, 0x21, 0x03, 0x10,     // LD IY,0x1003
        0xCB, 0x3E,                 // SRL (HL)
        0x7E,                       // LD A,(HL)
        0xDD, 0xCB, 0x01, 0x3E,     // SRL (IX+1)
        0xDD, 0x7E, 0x01,           // LD A,(IX+1)
        0xFD, 0xCB, 0xFF, 0x3E,     // SRL (IY-1)
        0xFD, 0x7E, 0xFF,           // LD A,(IY-1)
    };
    copy(0x1000, data, sizeof(data));
    copy(0x000, prog, sizeof(prog));
    init();
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(15==step()); T(0x00 == mem[0x1000]); T(flags(Z80_ZF|Z80_PF|Z80_CF));
    T(7 ==step()); T(0x00 == _A);
    T(23==step()); T(0x40 == mem[0x1001]); T(flags(0));
    T(19==step()); T(0x40 == _A);
    T(23==step()); T(0x55 == mem[0x1002]); T(flags(Z80_PF));
    T(19==step()); T(0x55 == _A);
}

/* RLD; RRD */
void RLD_RRD() {
    puts(">>> RLD; RRD");
    uint8_t prog[] = {
        0x3E, 0x12,         // LD A,0x12
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x36, 0x34,         // LD (HL),0x34
        0xED, 0x67,         // RRD
        0xED, 0x6F,         // RLD
        0x7E,               // LD A,(HL)
        0x3E, 0xFE,         // LD A,0xFE
        0x36, 0x00,         // LD (HL),0x00
        0xED, 0x6F,         // RLD
        0xED, 0x67,         // RRD
        0x7E,               // LD A,(HL)
        0x3E, 0x01,         // LD A,0x01
        0x36, 0x00,         // LD (HL),0x00
        0xED, 0x6F,         // RLD
        0xED, 0x67,         // RRD
        0x7E
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(7 ==step()); T(0x12 == _A);
    T(10==step()); T(0x1000 == _HL);
    T(10==step()); T(0x34 == mem[0x1000]);
    T(18==step()); T(0x14 == _A); T(0x23 == mem[0x1000]); T(0x1001 == _WZ);
    T(18==step()); T(0x12 == _A); T(0x34 == mem[0x1000]); T(0x1001 == _WZ);
    T(7 ==step()); T(0x34 == _A);
    T(7 ==step()); T(0xFE == _A);
    T(10==step()); T(0x00 == mem[0x1000]);
    T(18==step()); T(0xF0 == _A); T(0x0E == mem[0x1000]); T(flags(Z80_SF|Z80_PF)); T(0x1001 == _WZ);
    T(18==step()); T(0xFE == _A); T(0x00 == mem[0x1000]); T(flags(Z80_SF)); T(0x1001 == _WZ);
    T(7 ==step()); T(0x00 == _A);
    T(7 ==step()); T(0x01 == _A);
    T(10 ==step()); T(0x00 == mem[0x1000]);
    z80_set_f(&cpu, z80_f(&cpu) | Z80_CF);
    T(18==step()); T(0x00 == _A); T(0x01 == mem[0x1000]); T(flags(Z80_ZF|Z80_PF|Z80_CF)); T(0x1001 == _WZ);
    T(18==step()); T(0x01 == _A); T(0x00 == mem[0x1000]); T(flags(Z80_CF)); T(0x1001 == _WZ);
    T(7 ==step()); T(0x00 == _A);
}

/* HALT */
void HALT() {
    puts(">>> HALT");
    uint8_t prog[] = {
        0x76,       // HALT
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(4==step()); T(0x0000 == _PC); T(cpu.pins & Z80_HALT);
    T(4==step()); T(0x0000 == _PC); T(cpu.pins & Z80_HALT);
    T(4==step()); T(0x0000 == _PC); T(cpu.pins & Z80_HALT);
}

/* BIT b,r; BIT b,(HL); BIT b,(IX+d); BIT b,(IY+d) */
void BIT() {
    puts(" FIXME FIXME FIXME >>> BIT b,r; BIT b,(HL); BIT b,(IX+d); BIT b,(IY+d)");
    /* only test cycle count for now */
    uint8_t prog[] = {
        0xCB, 0x47,             // BIT 0,A
        0xCB, 0x46,             // BIT 0,(HL)
        0xDD, 0xCB, 0x01, 0x46, // BIT 0,(IX+1)
        0xFD, 0xCB, 0xFF, 0x46, // BIT 0,(IY-1)
        0xDD, 0xCB, 0x02, 0x47, // undocumented: BIT 0,(IX+2),A
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(8 ==step());
    T(12==step());
    T(20==step());
    T(20==step());
    T(20==step());
}

/* SET b,r; SET b,(HL); SET b,(IX+d); SET b,(IY+d) */
void SET() {
    puts(" FIXME FIXME FIXME >>> SET b,r; SET b,(HL); SET b,(IX+d); SET b,(IY+d)");
    /* only test cycle count for now */
    uint8_t prog[] = {
        0xCB, 0xC7,             // SET 0,A
        0xCB, 0xC6,             // SET 0,(HL)
        0xDD, 0xCB, 0x01, 0xC6, // SET 0,(IX+1)
        0xFD, 0xCB, 0xFF, 0xC6, // SET 0,(IY-1)
        0xDD, 0xCB, 0x02, 0xC7, // undocumented: SET 0,(IX+2),A
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(8 ==step());
    T(15==step());
    T(23==step());
    T(23==step());
    T(23==step());
}

/* RES b,r; RES b,(HL); RES b,(IX+d); RES b,(IY+d) */
void RES() {
    puts(" FIXME FIXME FIXME >>> RES b,r; RES b,(HL); RES b,(IX+d); RES b,(IY+d)");
    /* only test cycle count for now */
    uint8_t prog[] = {
        0xCB, 0x87,             // RES 0,A
        0xCB, 0x86,             // RES 0,(HL)
        0xDD, 0xCB, 0x01, 0x86, // RES 0,(IX+1)
        0xFD, 0xCB, 0xFF, 0x86, // RES 0,(IY-1)
        0xDD, 0xCB, 0x02, 0x87, // undocumented: RES 0,(IX+2),A
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(8 ==step());
    T(15==step());
    T(23==step());
    T(23==step());
    T(23==step());
}

/* DAA */
void DAA() {
    puts(">>> DAA");
    uint8_t prog[] = {
        0x3e, 0x15,         // ld a,0x15
        0x06, 0x27,         // ld b,0x27
        0x80,               // add a,b
        0x27,               // daa
        0x90,               // sub b
        0x27,               // daa
        0x3e, 0x90,         // ld a,0x90
        0x06, 0x15,         // ld b,0x15
        0x80,               // add a,b
        0x27,               // daa
        0x90,               // sub b
        0x27                // daa
    };
    copy(0x0000, prog, sizeof(prog));
    init();

    T(7==step()); T(0x15 == _A);
    T(7==step()); T(0x27 == _B);
    T(4==step()); T(0x3C == _A); T(flags(0));
    T(4==step()); T(0x42 == _A); T(flags(Z80_HF|Z80_PF));
    T(4==step()); T(0x1B == _A); T(flags(Z80_HF|Z80_NF));
    T(4==step()); T(0x15 == _A); T(flags(Z80_NF));
    T(7==step()); T(0x90 == _A); T(flags(Z80_NF));
    T(7==step()); T(0x15 == _B); T(flags(Z80_NF));
    T(4==step()); T(0xA5 == _A); T(flags(Z80_SF));
    T(4==step()); T(0x05 == _A); T(flags(Z80_PF|Z80_CF));
    T(4==step()); T(0xF0 == _A); T(flags(Z80_SF|Z80_NF|Z80_CF));
    T(4==step()); T(0x90 == _A); T(flags(Z80_SF|Z80_PF|Z80_NF|Z80_CF));
}

/* CPL */
void CPL() {
    puts(">>> CPL");
    uint8_t prog[] = {
        0x97,               // SUB A
        0x2F,               // CPL
        0x2F,               // CPL
        0xC6, 0xAA,         // ADD A,0xAA
        0x2F,               // CPL
        0x2F,               // CPL
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0xFF == _A); T(flags(Z80_ZF|Z80_HF|Z80_NF));
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_HF|Z80_NF));
    T(7==step()); T(0xAA == _A); T(flags(Z80_SF));
    T(4==step()); T(0x55 == _A); T(flags(Z80_SF|Z80_HF|Z80_NF));
    T(4==step()); T(0xAA == _A); T(flags(Z80_SF|Z80_HF|Z80_NF));
}

/* CCF/SCF */
void CCF_SCF() {
    puts(">>> CCF; SCF");
    uint8_t prog[] = {
        0x97,           // SUB A
        0x37,           // SCF
        0x3F,           // CCF
        0xD6, 0xCC,     // SUB 0xCC
        0x3F,           // CCF
        0x37,           // SCF
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_CF));
    T(4==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_HF));
    T(7==step()); T(0x34 == _A); T(flags(Z80_HF|Z80_NF|Z80_CF));
    T(4==step()); T(0x34 == _A); T(flags(Z80_HF));
    T(4==step()); T(0x34 == _A); T(flags(Z80_CF));
}

void NEG() {
    puts(">>> NEG");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0xED, 0x44,         // NEG
        0xC6, 0x01,         // ADD A,0x01
        0xED, 0x44,         // NEG
        0xD6, 0x80,         // SUB A,0x80
        0xED, 0x44,         // NEG
        0xC6, 0x40,         // ADD A,0x40
        0xED, 0x44,         // NEG
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(7==step()); T(0x01 == _A);
    T(8==step()); T(0xFF == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(7==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_HF|Z80_CF));
    T(8==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(7==step()); T(0x80 == _A); T(flags(Z80_SF|Z80_PF|Z80_NF|Z80_CF));
    T(8==step()); T(0x80 == _A); T(flags(Z80_SF|Z80_PF|Z80_NF|Z80_CF));
    T(7==step()); T(0xC0 == _A); T(flags(Z80_SF));
    T(8==step()); T(0x40 == _A); T(flags(Z80_NF|Z80_CF));
}

/* LDI */
void LDI() {
    puts(">>> LDI");
    uint8_t data[] = {
        0x01, 0x02, 0x03,
    };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0x11, 0x00, 0x20,       // LD DE,0x2000
        0x01, 0x03, 0x00,       // LD BC,0x0003
        0xED, 0xA0,             // LDI
        0xED, 0xA0,             // LDI
        0xED, 0xA0,             // LDI
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(16==step());
    T(0x1001 == _HL);
    T(0x2001 == _DE);
    T(0x0002 == _BC);
    T(0x01 == mem[0x2000]);
    T(flags(Z80_PF));
    T(16==step());
    T(0x1002 == _HL);
    T(0x2002 == _DE);
    T(0x0001 == _BC);
    T(0x02 == mem[0x2001]);
    T(flags(Z80_PF));
    T(16==step());
    T(0x1003 == _HL);
    T(0x2003 == _DE);
    T(0x0000 == _BC);
    T(0x03 == mem[0x2002]);
    T(flags(0));
}

/* LDIR */
void LDIR() {
    puts(">>> LDIR");
    uint8_t data[] = {
        0x01, 0x02, 0x03,
    };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0x11, 0x00, 0x20,       // LD DE,0x2000
        0x01, 0x03, 0x00,       // LD BC,0x0003
        0xED, 0xB0,             // LDIR
        0x3E, 0x33,             // LD A,0x33
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(21==step());
    T(0x1001 == _HL);
    T(0x2001 == _DE);
    T(0x0002 == _BC);
    T(0x000A == _WZ);
    T(0x01 == mem[0x2000]);
    T(flags(Z80_PF));
    T(21==step());
    T(0x1002 == _HL);
    T(0x2002 == _DE);
    T(0x0001 == _BC);
    T(0x000A == _WZ);
    T(0x02 == mem[0x2001]);
    T(flags(Z80_PF));
    T(16==step());
    T(0x1003 == _HL);
    T(0x2003 == _DE);
    T(0x0000 == _BC);
    T(0x02 == mem[0x2001]);
    T(0x03 == mem[0x2002]);
    T(flags(0));
    T(7==step()); T(0x33 == _A);
}

/* LDD */
void LDD() {
    puts(">>> LDD");
    uint8_t data[] = {
        0x01, 0x02, 0x03,
    };
    uint8_t prog[] = {
        0x21, 0x02, 0x10,       // LD HL,0x1002
        0x11, 0x02, 0x20,       // LD DE,0x2002
        0x01, 0x03, 0x00,       // LD BC,0x0003
        0xED, 0xA8,             // LDD
        0xED, 0xA8,             // LDD
        0xED, 0xA8,             // LDD
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(16==step());
    T(0x1001 == _HL);
    T(0x2001 == _DE);
    T(0x0002 == _BC);
    T(0x03 == mem[0x2002]);
    T(flags(Z80_PF));
    T(16==step());
    T(0x1000 == _HL);
    T(0x2000 == _DE);
    T(0x0001 == _BC);
    T(0x02 == mem[0x2001]);
    T(flags(Z80_PF));
    T(step());
    T(0x0FFF == _HL);
    T(0x1FFF == _DE);
    T(0x0000 == _BC);
    T(0x01 == mem[0x2000]);
    T(flags(0));
}

/* LDDR */
void LDDR() {
    puts(">>> LDDR");
    uint8_t data[] = {
        0x01, 0x02, 0x03,
    };
    uint8_t prog[] = {
        0x21, 0x02, 0x10,       // LD HL,0x1002
        0x11, 0x02, 0x20,       // LD DE,0x2002
        0x01, 0x03, 0x00,       // LD BC,0x0003
        0xED, 0xB8,             // LDDR
        0x3E, 0x33,             // LD A,0x33
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(21==step());
    T(0x1001 == _HL);
    T(0x2001 == _DE);
    T(0x0002 == _BC);
    T(0x000A == _WZ);
    T(0x03 == mem[0x2002]);
    T(flags(Z80_PF));
    T(21==step());
    T(0x1000 == _HL);
    T(0x2000 == _DE);
    T(0x0001 == _BC);
    T(0x000A == _WZ);
    T(0x02 == mem[0x2001]);
    T(flags(Z80_PF));
    T(16==step());
    T(0x0FFF == _HL);
    T(0x1FFF == _DE);
    T(0x0000 == _BC);
    T(0x000A == _WZ);
    T(0x01 == mem[0x2000]);
    T(flags(0));
    T(7 == step()); T(0x33 == _A);
}

/* CPI */
void CPI() {
    puts(">>> CPI");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // ld hl,0x1000
        0x01, 0x04, 0x00,       // ld bc,0x0004
        0x3e, 0x03,             // ld a,0x03
        0xed, 0xa1,             // cpi
        0xed, 0xa1,             // cpi
        0xed, 0xa1,             // cpi
        0xed, 0xa1,             // cpi
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(16 == step());
    T(0x1001 == _HL);
    T(0x0003 == _BC);
    T(flags(Z80_PF|Z80_NF));
    z80_set_f(&cpu, z80_f(&cpu) | Z80_CF);
    T(16 == step());
    T(0x1002 == _HL);
    T(0x0002 == _BC);
    T(flags(Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1003 == _HL);
    T(0x0001 == _BC);
    T(flags(Z80_ZF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1004 == _HL);
    T(0x0000 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
}

/* CPIR */
void CPIR() {
    puts(">>> CPIR");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // ld hl,0x1000
        0x01, 0x04, 0x00,       // ld bc,0x0004
        0x3e, 0x03,             // ld a,0x03
        0xed, 0xb1,             // cpir
        0xed, 0xb1,             // cpir
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0003 == _BC);
    T(flags(Z80_PF|Z80_NF));
    z80_set_f(&cpu, z80_f(&cpu) | Z80_CF);
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0002 == _BC);
    T(flags(Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1003 == _HL);
    T(0x0001 == _BC);
    T(flags(Z80_ZF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1004 == _HL);
    T(0x0000 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
}

/* CPD */
void CPD() {
    puts(">>> CPD");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };
    uint8_t prog[] = {
        0x21, 0x03, 0x10,       // ld hl,0x1004
        0x01, 0x04, 0x00,       // ld bc,0x0004
        0x3e, 0x02,             // ld a,0x03
        0xed, 0xa9,             // cpi
        0xed, 0xa9,             // cpi
        0xed, 0xa9,             // cpi
        0xed, 0xa9,             // cpi
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(16 == step());
    T(0x1002 == _HL);
    T(0x0003 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_PF|Z80_NF));
    z80_set_f(&cpu, z80_f(&cpu) | Z80_CF);
    T(16 == step());
    T(0x1001 == _HL);
    T(0x0002 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1000 == _HL);
    T(0x0001 == _BC);
    T(flags(Z80_ZF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x0FFF == _HL);
    T(0x0000 == _BC);
    T(flags(Z80_NF|Z80_CF));
}

/* CPDR */
void CPDR() {
    puts(">>> CPDR");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };
    uint8_t prog[] = {
        0x21, 0x03, 0x10,       // ld hl,0x1004
        0x01, 0x04, 0x00,       // ld bc,0x0004
        0x3e, 0x02,             // ld a,0x03
        0xed, 0xb9,             // cpdr
        0xed, 0xb9,             // cpdr
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();

    // skip loads
    for (int i = 0; i < 3; i++) {
        step();
    }
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0003 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_PF|Z80_NF));
    z80_set_f(&cpu, z80_f(&cpu) | Z80_CF);
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0002 == _BC);
    T(flags(Z80_SF|Z80_HF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x1000 == _HL);
    T(0x0001 == _BC);
    T(flags(Z80_ZF|Z80_PF|Z80_NF|Z80_CF));
    T(16 == step());
    T(0x0FFF == _HL);
    T(0x0000 == _BC);
    T(flags(Z80_NF|Z80_CF));
}

/* DI/EI/IM */
void DI_EI_IM() {
    puts(">>> DI; EI; IM");
    uint8_t prog[] = {
        0xF3,           // DI
        0xFB,           // EI
        0x00,           // NOP
        0xF3,           // DI
        0xFB,           // EI
        0x00,           // NOP
        0xED, 0x46,     // IM 0
        0xED, 0x56,     // IM 1
        0xED, 0x5E,     // IM 2
        0xED, 0x46,     // IM 0
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(4==step()); T(!z80_iff1(&cpu)); T(!z80_iff2(&cpu));
    T(4==step()); T(z80_iff1(&cpu));  T(z80_iff2(&cpu));
    T(4==step()); T(z80_iff1(&cpu));  T(z80_iff2(&cpu));
    T(4==step()); T(!z80_iff1(&cpu)); T(!z80_iff2(&cpu));
    T(4==step()); T(z80_iff1(&cpu));  T(z80_iff2(&cpu));
    T(4==step()); T(z80_iff1(&cpu));  T(z80_iff2(&cpu));
    T(8==step()); T(0 == z80_im(&cpu));
    T(8==step()); T(1 == z80_im(&cpu));
    T(8==step()); T(2 == z80_im(&cpu));
    T(8==step()); T(0 == z80_im(&cpu));
}

/* JP cc,nn */
void JP_cc_nn() {
    puts(">>> JP cc,nn");
    uint8_t prog[] = {
        0x97,               //          SUB A
        0xC2, 0x0C, 0x02,   //          JP NZ,label0
        0xCA, 0x0C, 0x02,   //          JP Z,label0
        0x00,               //          NOP
        0xC6, 0x01,         // label0:  ADD A,0x01
        0xCA, 0x15, 0x02,   //          JP Z,label1
        0xC2, 0x15, 0x02,   //          JP NZ,label1
        0x00,               //          NOP
        0x07,               // label1:  RLCA
        0xEA, 0x1D, 0x02,   //          JP PE,label2
        0xE2, 0x1D, 0x02,   //          JP PO,label2
        0x00,               //          NOP
        0xC6, 0xFD,         // label2:  ADD A,0xFD
        0xF2, 0x26, 0x02,   //          JP P,label3
        0xFA, 0x26, 0x02,   //          JP M,label3
        0x00,               //          NOP
        0xD2, 0x2D, 0x02,   // label3:  JP NC,label4
        0xDA, 0x2D, 0x02,   //          JP C,label4
        0x00,               //          NOP
        0x00,               //          NOP
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_pc(&cpu, 0x0204);
    
    T(4 ==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(10==step()); T(0x0208 == _PC); T(0x020C == _WZ);
    T(10==step()); T(0x020C == _PC); T(0x020C == _WZ);
    T(7 ==step()); T(0x01 == _A); T(flags(0));
    T(10==step()); T(0x0211 == _PC);
    T(10==step()); T(0x0215 == _PC);
    T(4 ==step()); T(0x02 == _A); T(flags(0));
    T(10==step()); T(0x0219 == _PC);
    T(10==step()); T(0x021D == _PC);
    T(7 ==step()); T(0xFF == _A); T(flags(Z80_SF));
    T(10==step()); T(0x0222 == _PC);
    T(10==step()); T(0x0226 == _PC);
    T(10==step()); T(0x022D == _PC);
}

/* JP; JR */
void JP_JR() {
    puts(">>> JP; JR");
    uint8_t prog[] = {
        0x21, 0x16, 0x02,           //      LD HL,l3
        0xDD, 0x21, 0x19, 0x02,     //      LD IX,l4
        0xFD, 0x21, 0x21, 0x02,     //      LD IY,l5
        0xC3, 0x14, 0x02,           //      JP l0
        0x18, 0x04,                 // l1:  JR l2
        0x18, 0xFC,                 // l0:  JR l1
        0xDD, 0xE9,                 // l3:  JP (IX)
        0xE9,                       // l2:  JP (HL)
        0xFD, 0xE9,                 // l4:  JP (IY)
        0x18, 0x06,                 // l6:  JR l7
        0x00, 0x00, 0x00, 0x00,     //      4x NOP
        0x18, 0xF8,                 // l5:  JR l6
        0x00                        // l7:  NOP
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_pc(&cpu, 0x0204);
    T(10==step()); T(0x0216 == _HL);
    T(14==step()); T(0x0219 == _IX);
    T(14==step()); T(0x0221 == _IY);
    T(10==step()); T(0x0214 == _PC); T(0x0214 == _WZ);
    T(12==step()); T(0x0212 == _PC); T(0x0212 == _WZ);
    T(12==step()); T(0x0218 == _PC); T(0x0218 == _WZ);
    T(4 ==step()); T(0x0216 == _PC); T(0x0218 == _WZ);
    T(8 ==step()); T(0x0219 == _PC); T(0x0218 == _WZ);
    T(8 ==step()); T(0x0221 == _PC); T(0x0218 == _WZ);
    T(12==step()); T(0x021B == _PC); T(0x021B == _WZ);
    T(12==step()); T(0x0223 == _PC); T(0x0223 == _WZ);
}

/* JR_cc_e) */
void JR_cc_e() {
    puts(">>> JR cc,e");
    uint8_t prog[] = {
        0x97,           //      SUB A
        0x20, 0x03,     //      JR NZ,l0
        0x28, 0x01,     //      JR Z,l0
        0x00,           //      NOP
        0xC6, 0x01,     // l0:  ADD A,0x01
        0x28, 0x03,     //      JR Z,l1
        0x20, 0x01,     //      JR NZ,l1
        0x00,           //      NOP
        0xD6, 0x03,     // l1:  SUB 0x03
        0x30, 0x03,     //      JR NC,l2
        0x38, 0x01,     //      JR C,l2
        0x00,           //      NOP
        0x00,           // l2:  NOP
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_pc(&cpu, 0x0204);
    T(4 ==step()); T(0x00 == _A); T(flags(Z80_ZF|Z80_NF));
    T(7 ==step()); T(0x0207 == _PC);
    T(12==step()); T(0x020A == _PC); T(0x020A == _WZ);
    T(7 ==step()); T(0x01 == _A); T(flags(0));
    T(7 ==step()); T(0x020E == _PC);
    T(12==step()); T(0x0211 == _PC); T(0x0211 == _WZ);
    T(7 ==step()); T(0xFE == _A); T(flags(Z80_SF|Z80_HF|Z80_NF|Z80_CF));
    T(7 ==step()); T(0x0215 == _PC);
    T(12==step()); T(0x0218 == _PC); T(0x0218 == _WZ);
}

/* DJNZ */
void DJNZ() {
    puts(">>> DJNZ");
    uint8_t prog[] = {
        0x06, 0x03,         //      LD B,0x03
        0x97,               //      SUB A
        0x3C,               // l0:  INC A
        0x10, 0xFD,         //      DJNZ l0
        0x00,               //      NOP
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_pc(&cpu, 0x0204);
    T(7 ==step()); T(0x03 == _B);
    T(4 ==step()); T(0x00 == _A);
    T(4 ==step()); T(0x01 == _A);
    T(13==step()); T(0x02 == _B); T(0x0207 == _PC); T(0x0207 == _WZ);
    T(4 ==step()); T(0x02 == _A);
    T(13==step()); T(0x01 == _B); T(0x0207 == _PC); T(0x0207 == _WZ);
    T(4 ==step()); T(0x03 == _A);
    T(8 ==step()); T(0x00 == _B); T(0x020A == _PC); T(0x0207 == _WZ);
}

/* CALL; RET */
void CALL_RET() {
    puts(">>> CALL; RET");
    uint8_t prog[] = {
        0xCD, 0x0A, 0x02,       //      CALL l0
        0xCD, 0x0A, 0x02,       //      CALL l0
        0xC9,                   // l0:  RET
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_sp(&cpu, 0x0100);
    z80_set_pc(&cpu, 0x0204);
    T(17 == step());
    T(0x020A == _PC); T(0x020A == _WZ); T(0x00FE == _SP);
    T(0x07 == mem[0x00FE]); T(0x02 == mem[0x00FF]);
    T(10 == step());
    T(0x0207 == _PC); T(0x0207 == _WZ); T(0x0100 == _SP);
    T(17 == step());
    T(0x020A == _PC); T(0x020A == _WZ); T(0x00FE == _SP);
    T(0x0A == mem[0x00FE]); T(0x02 == mem[0x00FF]);
    T(10 == step());
    T(0x020A == _PC); T(0x020A == _WZ); T(0x0100 == _SP);
}

/* CALL cc/RET cc */
void CALL_RET_cc() {
    puts(">>> CALL cc; RET cc");
    uint8_t prog[] = {
        0x97,               //      SUB A
        0xC4, 0x29, 0x02,   //      CALL NZ,l0
        0xCC, 0x29, 0x02,   //      CALL Z,l0
        0xC6, 0x01,         //      ADD A,0x01
        0xCC, 0x2B, 0x02,   //      CALL Z,l1
        0xC4, 0x2B, 0x02,   //      CALL NZ,l1
        0x07,               //      RLCA
        0xEC, 0x2D, 0x02,   //      CALL PE,l2
        0xE4, 0x2D, 0x02,   //      CALL PO,l2
        0xD6, 0x03,         //      SUB 0x03
        0xF4, 0x2F, 0x02,   //      CALL P,l3
        0xFC, 0x2F, 0x02,   //      CALL M,l3
        0xD4, 0x31, 0x02,   //      CALL NC,l4
        0xDC, 0x31, 0x02,   //      CALL C,l4
        0xC9,               //      RET
        0xC0,               // l0:  RET NZ
        0xC8,               //      RET Z
        0xC8,               // l1:  RET Z
        0xC0,               //      RET NZ
        0xE8,               // l2:  RET PE
        0xE0,               //      RET PO
        0xF0,               // l3:  RET P
        0xF8,               //      RET M
        0xD0,               // l4:  RET NC
        0xD8,               //      RET C
    };
    copy(0x0204, prog, sizeof(prog));
    init();
    z80_set_pc(&cpu, 0x0204);
    z80_set_sp(&cpu, 0x0100);
    T(4 ==step()); T(0x00 == _A);
    T(10==step()); T(0x0208 == _PC); T(0x0229 == _WZ);
    T(17==step()); T(0x0229 == _PC); T(0x0229 == _WZ);
    T(5 ==step()); T(0x022A == _PC); T(0x0229 == _WZ);
    T(11==step()); T(0x020B == _PC); T(0x020B == _WZ);
    T(7 ==step()); T(0x01 == _A);
    T(10==step()); T(0x0210 == _PC);
    T(17==step()); T(0x022B == _PC);
    T(5 ==step()); T(0x022C == _PC);
    T(11==step()); T(0x0213 == _PC);
    T(4 ==step()); T(0x02 == _A);
    T(10==step()); T(0x0217 == _PC);
    T(17==step()); T(0x022D == _PC);
    T(5 ==step()); T(0x022E == _PC);
    T(11==step()); T(0x021A == _PC);
    T(7 ==step()); T(0xFF == _A);
    T(10==step()); T(0x021F == _PC);
    T(17==step()); T(0x022F == _PC);
    T(5 ==step()); T(0x0230 == _PC);
    T(11==step()); T(0x0222 == _PC);
    T(10==step()); T(0x0225 == _PC);
    T(17==step()); T(0x0231 == _PC);
    T(5 ==step()); T(0x0232 == _PC);
    T(11==step()); T(0x0228 == _PC);
}

/* ADD HL,rr; ADC HL,rr; SBC HL,rr; ADD IX,rr; ADD IY,rr */
void ADD_ADC_SBC_16() {
    puts(">>> ADD HL,rr; ADC HL,rr; SBC HL,rr; ADD IX,rr; ADD IY,rr");
    uint8_t prog[] = {
        0x21, 0xFC, 0x00,       // LD HL,0x00FC
        0x01, 0x08, 0x00,       // LD BC,0x0008
        0x11, 0xFF, 0xFF,       // LD DE,0xFFFF
        0x09,                   // ADD HL,BC
        0x19,                   // ADD HL,DE
        0xED, 0x4A,             // ADC HL,BC
        0x29,                   // ADD HL,HL
        0x19,                   // ADD HL,DE
        0xED, 0x42,             // SBC HL,BC
        0xDD, 0x21, 0xFC, 0x00, // LD IX,0x00FC
        0x31, 0x00, 0x10,       // LD SP,0x1000
        0xDD, 0x09,             // ADD IX, BC
        0xDD, 0x19,             // ADD IX, DE
        0xDD, 0x29,             // ADD IX, IX
        0xDD, 0x39,             // ADD IX, SP
        0xFD, 0x21, 0xFF, 0xFF, // LD IY,0xFFFF
        0xFD, 0x09,             // ADD IY,BC
        0xFD, 0x19,             // ADD IY,DE
        0xFD, 0x29,             // ADD IY,IY
        0xFD, 0x39,             // ADD IY,SP
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10==step()); T(0x00FC == _HL);
    T(10==step()); T(0x0008 == _BC);
    T(10==step()); T(0xFFFF == _DE);
    T(11==step()); T(0x0104 == _HL); T(flags(0)); T(0x00FD == _WZ);
    T(11==step()); T(0x0103 == _HL); T(flags(Z80_HF|Z80_CF)); T(0x0105 == _WZ);
    T(15==step()); T(0x010C == _HL); T(flags(0)); T(0x0104 == _WZ);
    T(11==step()); T(0x0218 == _HL); T(flags(0)); T(0x010D == _WZ);
    T(11==step()); T(0x0217 == _HL); T(flags(Z80_HF|Z80_CF)); T(0x0219 == _WZ);
    T(15==step()); T(0x020E == _HL); T(flags(Z80_NF)); T(0x0218 == _WZ);
    T(14==step()); T(0x00FC == _IX);
    T(10==step()); T(0x1000 == _SP);
    T(15==step()); T(0x0104 == _IX); T(flags(0)); T(0x00FD == _WZ);
    T(15==step()); T(0x0103 == _IX); T(flags(Z80_HF|Z80_CF)); T(0x0105 == _WZ);
    T(15==step()); T(0x0206 == _IX); T(flags(0)); T(0x0104 == _WZ);
    T(15==step()); T(0x1206 == _IX); T(flags(0)); T(0x0207 == _WZ);
    T(14==step()); T(0xFFFF == _IY); 
    T(15==step()); T(0x0007 == _IY); T(flags(Z80_HF|Z80_CF)); T(0x0000 == _WZ);
    T(15==step()); T(0x0006 == _IY); T(flags(Z80_HF|Z80_CF)); T(0x0008 == _WZ);
    T(15==step()); T(0x000C == _IY); T(flags(0)); T(0x0007 == _WZ);
    T(15==step()); T(0x100C == _IY); T(flags(0)); T(0x000D == _WZ);
}

/* IN */
void IN() {
    puts(">>> IN");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0xDB, 0x03,         // IN A,(0x03)
        0xDB, 0x04,         // IN A,(0x04)
        0x01, 0x02, 0x02,   // LD BC,0x0202
        0xED, 0x78,         // IN A,(C)
        0x01, 0xFF, 0x05,   // LD BC,0x05FF
        0xED, 0x50,         // IN D,(C)
        0x01, 0x05, 0x05,   // LD BC,0x0505
        0xED, 0x58,         // IN E,(C)
        0x01, 0x06, 0x01,   // LD BC,0x0106
        0xED, 0x60,         // IN H,(C)
        0x01, 0x00, 0x10,   // LD BC,0x0000
        0xED, 0x68,         // IN L,(C)
        0xED, 0x40,         // IN B,(C)
        0xED, 0x48,         // IN C,(c)
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    z80_set_f(&cpu, Z80_HF|Z80_CF);
    T(7 ==step()); T(0x01 == _A); T(flags(Z80_HF|Z80_CF));
    T(11==step()); T(0x06 == _A); T(flags(Z80_HF|Z80_CF)); T(0x0104 == _WZ);
    T(11==step()); T(0x08 == _A); T(flags(Z80_HF|Z80_CF)); T(0x0605 == _WZ);
    T(10==step()); T(0x0202 == _BC);
    T(12==step()); T(0x04 == _A); T(flags(Z80_CF)); T(0x0203 == _WZ);
    T(10==step()); T(0x05FF == _BC);
    T(12==step()); T(0xFE == _D); T(flags(Z80_SF|Z80_CF)); T(0x0600 == _WZ);
    T(10==step()); T(0x0505 == _BC);
    T(12==step()); T(0x0A == _E); T(flags(Z80_PF|Z80_CF)); T(0x0506 == _WZ);
    T(10==step()); T(0x0106 == _BC);
    T(12==step()); T(0x0C == _H); T(flags(Z80_PF|Z80_CF)); T(0x0107 == _WZ);
    T(10==step()); T(0x1000 == _BC);
    T(12==step()); T(0x00 == _L); T(flags(Z80_ZF|Z80_PF|Z80_CF)); T(0x1001 == _WZ);
    T(12==step()); T(0x00 == _B); T(flags(Z80_ZF|Z80_PF|Z80_CF)); T(0x1001 == _WZ);
    T(12==step()); T(0x00 == _C); T(flags(Z80_ZF|Z80_PF|Z80_CF)); T(0x0001 == _WZ);
}

/* OUT */
void OUT() {
    puts(">>> OUT");
    uint8_t prog[] = {
        0x3E, 0x01,         // LD A,0x01
        0xD3, 0x01,         // OUT (0x01),A
        0xD3, 0xFF,         // OUT (0xFF),A
        0x01, 0x34, 0x12,   // LD BC,0x1234
        0x11, 0x78, 0x56,   // LD DE,0x5678
        0x21, 0xCD, 0xAB,   // LD HL,0xABCD
        0xED, 0x79,         // OUT (C),A
        0xED, 0x41,         // OUT (C),B
        0xED, 0x49,         // OUT (C),C
        0xED, 0x51,         // OUT (C),D
        0xED, 0x59,         // OUT (C),E
        0xED, 0x61,         // OUT (C),H
        0xED, 0x69,         // OUT (C),L
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(7 ==step()); T(0x01 == _A);
    T(11==step()); T(0x0101 == out_port); T(0x01 == out_byte); T(0x0102 == _WZ);
    T(11==step()); T(0x01FF == out_port); T(0x01 == out_byte); T(0x0100 == _WZ);
    T(10==step()); T(0x1234 == _BC);
    T(10==step()); T(0x5678 == _DE);
    T(10==step()); T(0xABCD == _HL);
    T(12==step()); T(0x1234 == out_port); T(0x01 == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0x12 == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0x34 == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0x56 == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0x78 == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0xAB == out_byte); T(0x1235 == _WZ);
    T(12==step()); T(0x1234 == out_port); T(0xCD == out_byte); T(0x1235 == _WZ);
}

/* INIR; INDR */
void INIR_INDR() {
    puts(">>> INIR; INDR");
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0x01, 0x02, 0x03,       // LD BC,0x0302
        0xED, 0xB2,             // INIR
        0x01, 0x03, 0x03,       // LD BC,0x0303
        0xED, 0xBA              // INDR
    };
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10 == step()); T(0x1000 == _HL);
    T(10 == step()); T(0x0302 == _BC);
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0202 == _BC);
    T(0x04 == mem[0x1000]);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0102 == _BC);
    T(0x04 == mem[0x1001]);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(16 == step());
    T(0x1003 == _HL);
    T(0x0002 == _BC);
    T(0x04 == mem[0x1002]);
    T(z80_f(&cpu) & Z80_ZF);
    T(10 == step()); T(0x0303 == _BC);
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0203 == _BC);
    T(0x06 == mem[0x1003]);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0103 == _BC);
    T(0x06 == mem[0x1002]);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(16 == step());
    T(0x1000 == _HL);
    T(0x0003 == _BC);
    T(0x06 == mem[0x1001]);
    T(z80_f(&cpu) & Z80_ZF);
}

/* OTIR; OTDR */
void OTIR_OTDR() {
    puts(">>> OTIR; OTDR");
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };
    uint8_t prog[] = {
        0x21, 0x00, 0x10,       // LD HL,0x1000
        0x01, 0x02, 0x03,       // LD BC,0x0302
        0xED, 0xB3,             // OTIR
        0x01, 0x03, 0x03,       // LD BC,0x0303
        0xED, 0xBB,             // OTDR
    };
    copy(0x1000, data, sizeof(data));
    copy(0x0000, prog, sizeof(prog));
    init();
    T(10 == step()); T(0x1000 == _HL);
    T(10 == step()); T(0x0302 == _BC);
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0202 == _BC);
    T(0x0202 == out_port); T(0x01 == out_byte);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0102 == _BC);
    T(0x0102 == out_port); T(0x02 == out_byte);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(16 == step());
    T(0x1003 == _HL);
    T(0x0002 == _BC);
    T(0x0002 == out_port); T(0x03 == out_byte);
    T(z80_f(&cpu) & Z80_ZF);
    T(10 == step()); T(0x0303 == _BC);
    T(21 == step());
    T(0x1002 == _HL);
    T(0x0203 == _BC);
    T(0x0203 == out_port); T(0x04 == out_byte);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(21 == step());
    T(0x1001 == _HL);
    T(0x0103 == _BC);
    T(0x0103 == out_port); T(0x03 == out_byte);
    T(!(z80_f(&cpu) & Z80_ZF));
    T(16 == step());
    T(0x1000 == _HL);
    T(0x0003 == _BC);
    T(0x0003 == out_port); T(0x02 == out_byte);
    T(z80_f(&cpu) & Z80_ZF);
}

int main() {
    SET_GET();
    LD_A_RI();
    LD_IR_A();
    LD_r_sn();
    LD_r_iHLi();
    LD_r_iIXIYi();
    LD_iHLi_r();
    LD_iIXIYi_r();
    LD_iHLi_n();
    LD_iIXIYi_n();
    LD_A_iBCDEnni();
    LD_iBCDEnni_A();
    LD_ddIXIY_nn();
    LD_HLddIXIY_inni();
    LD_inni_HLddIXIY();
    LD_SP_HLIXIY();
    PUSH_POP_qqIXIY();
    EX();
    ADD_A_rn();
    ADD_A_iHLIXIYi();
    ADC_A_rn();
    ADC_A_iHLIXIYi();
    SUB_A_rn();
    SUB_A_iHLIXIYi();
    SBC_A_rn();
    SBC_A_iHLIXIYi();
    CP_A_rn();
    CP_A_iHLIXIYi();
    AND_A_rn();
    AND_A_iHLIXIYi();
    XOR_A_rn();
    OR_A_rn();
    OR_XOR_A_iHLIXIYi();
    INC_DEC_r();
    INC_DEC_iHLIXIYi();
    INC_DEC_ssIXIY();
    RLCA_RLA_RRCA_RRA();
    RLC_RL_RRC_RR_r();
    RRC_RLC_RR_RL_iHLIXIYi();
    SLA_r();
    SLA_iHLIXIYi();
    SRA_r();
    SRA_iHLIXIYi();
    SRL_r();
    SRL_iHLIXIYi();
    RLD_RRD();
    HALT();
    BIT();
    SET();
    RES();
    DAA();
    CPL();
    CCF_SCF();
    NEG();
    LDI();
    LDIR();
    LDD();
    LDDR();
    CPI();
    CPIR();
    CPD();
    CPDR();
    DI_EI_IM();
    JP_cc_nn();
    JP_JR();
    JR_cc_e();
    DJNZ();
    CALL_RET();
    CALL_RET_cc();
    ADD_ADC_SBC_16();
    IN();
    OUT();
    INIR_INDR();
    OTIR_OTDR();
    printf("%d tests run ok.\n", num_tests);
    return 0;
}

