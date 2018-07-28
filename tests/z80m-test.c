//------------------------------------------------------------------------------
//  z80m-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80m.h"
#include <stdio.h>

#define _A z80m_a(&cpu)
#define _F z80m_f(&cpu)
#define _L z80m_l(&cpu)
#define _H z80m_h(&cpu)
#define _E z80m_e(&cpu)
#define _D z80m_d(&cpu)
#define _C z80m_c(&cpu)
#define _B z80m_b(&cpu)
#define _FA z80m_fa(&cpu)
#define _HL z80m_hl(&cpu)
#define _DE z80m_de(&cpu)
#define _BC z80m_bc(&cpu)
#define _FA_ z80m_fa_(&cpu)
#define _HL_ z80m_hl_(&cpu)
#define _DE_ z80m_de_(&cpu)
#define _BC_ z80m_bc_(&cpu)
#define _PC z80m_pc(&cpu)
#define _SP z80m_sp(&cpu)
#define _WZ z80m_wz(&cpu)
#define _IR z80m_ir(&cpu)
#define _I z80m_i(&cpu)
#define _R z80m_r(&cpu)
#define _IX z80m_ix(&cpu)
#define _IY z80m_iy(&cpu)
#define _IM z80m_im(&cpu)
#define _IFF1 z80m_iff1(&cpu)
#define _IFF2 z80m_iff2(&cpu)
#define _EI_PENDING z80m_ei_pending(&cpu)

z80m_t cpu;
uint8_t mem[1<<16] = { 0 };
uint16_t out_port = 0;
uint8_t out_byte = 0;
uint64_t tick(int num, uint64_t pins, void* user_data) {
    if (pins & Z80M_MREQ) {
        if (pins & Z80M_RD) {
            /* memory read */
            Z80M_SET_DATA(pins, mem[Z80M_GET_ADDR(pins)]);
        }
        else if (pins & Z80M_WR) {
            /* memory write */
            mem[Z80M_GET_ADDR(pins)] = Z80M_GET_DATA(pins);
        }
    }
    else if (pins & Z80M_IORQ) {
        if (pins & Z80M_RD) {
            /* IN */
            Z80M_SET_DATA(pins, (Z80M_GET_ADDR(pins) & 0xFF) * 2);
        }
        else if (pins & Z80M_WR) {
            /* OUT */
            out_port = Z80M_GET_ADDR(pins);
            out_byte = Z80M_GET_DATA(pins);
        }
    }
    return pins;
}

void init() {
    z80m_init(&cpu, &(z80m_desc_t) { .tick_cb = tick, });
    z80m_set_f(&cpu, 0);
    z80m_set_fa_(&cpu, 0xFF00);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return z80m_exec(&cpu, 0);
}

bool flags(uint8_t expected) {
    /* don't check undocumented flags */
    return (z80m_f(&cpu) & ~(Z80M_YF|Z80M_XF)) == expected;
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
    z80m_set_f(&cpu, 0x01); z80m_set_a(&cpu, 0x23);
    z80m_set_h(&cpu, 0x45); z80m_set_l(&cpu, 0x67);
    z80m_set_d(&cpu, 0x89); z80m_set_e(&cpu, 0xAB);
    z80m_set_b(&cpu, 0xCD); z80m_set_c(&cpu, 0xEF);
    T(0x01 == _F); T(0x23 == _A); T(0x0123 == _FA);
    T(0x45 == _H); T(0x67 == _L); T(0x4567 == _HL);
    T(0x89 == _D); T(0xAB == _E); T(0x89AB == _DE);
    T(0xCD == _B); T(0xEF == _C); T(0xCDEF == _BC);

    z80m_set_fa(&cpu, 0x1234);
    z80m_set_hl(&cpu, 0x5678);
    z80m_set_de(&cpu, 0x9abc);
    z80m_set_bc(&cpu, 0xdef0);
    z80m_set_fa_(&cpu, 0x4321);
    z80m_set_hl_(&cpu, 0x8765);
    z80m_set_de_(&cpu, 0xCBA9);
    z80m_set_bc_(&cpu, 0x0FED);
    T(0x12 == _F); T(0x34 == _A); T(0x1234 == _FA);
    T(0x56 == _H); T(0x78 == _L); T(0x5678 == _HL);
    T(0x9A == _D); T(0xBC == _E); T(0x9ABC == _DE);
    T(0xDE == _B); T(0xF0 == _C); T(0xDEF0 == _BC);
    T(0x4321 == _FA_);
    T(0x8765 == _HL_);
    T(0xCBA9 == _DE_);
    T(0x0FED == _BC_);

    z80m_set_pc(&cpu, 0xCEDF);
    z80m_set_wz(&cpu, 0x1324);
    z80m_set_sp(&cpu, 0x2435);
    z80m_set_i(&cpu, 0x35);
    z80m_set_r(&cpu, 0x46);
    z80m_set_ix(&cpu, 0x4657);
    z80m_set_iy(&cpu, 0x5768);
    T(0xCEDF == _PC);
    T(0x1324 == _WZ);
    T(0x2435 == _SP);
    T(0x35 == _I);
    T(0x46 == _R);
    T(0x4657 == _IX); 
    T(0x5768 == _IY);

    z80m_set_im(&cpu, 2);
    z80m_set_iff1(&cpu, true);
    z80m_set_iff2(&cpu, false);
    z80m_set_ei_pending(&cpu, true);
    T(2 == _IM);
    T(_IFF1);
    T(!_IFF2);
    T(_EI_PENDING);
    z80m_set_iff1(&cpu, false);
    z80m_set_iff2(&cpu, true);
    z80m_set_ei_pending(&cpu, false);
    T(!_IFF1);
    T(_IFF2);
    T(!_EI_PENDING);
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

int main() {
    SET_GET();
    LD_r_sn();
    LD_r_iHLi();
    LD_iHLi_r();
    LD_iHLi_n();
    LD_A_iBCDEnni();
    LD_iBCDEnni_A();
    printf("%d tests run ok.\n", num_tests);
    return 0;
}

