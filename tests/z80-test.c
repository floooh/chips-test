//------------------------------------------------------------------------------
//  z80-test.c
//------------------------------------------------------------------------------
#ifndef _DEBUG
#define _DEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80.h"

uint8_t mem[1<<16] = { 0 };

void tick(z80* cpu) {
    if (cpu->CTRL & Z80_MREQ) {
        if (cpu->CTRL & Z80_RD) {
            /* memory read */
            cpu->DATA = mem[cpu->ADDR];
        }
        else if (cpu->CTRL & Z80_WR) {
            /* memory write */
            mem[cpu->ADDR] = cpu->DATA;
        }
    }
    else if (cpu->CTRL & Z80_IORQ) {
        if (cpu->CTRL & Z80_RD) {
            /* IN */
        }
        else if (cpu->CTRL & Z80_WR) {
            /* OUT */
        }
    }
}

z80 cpu;
void init() {
    z80_desc desc = {
        .tick_func = tick,
        .tick_context = 0
    };
    z80_init(&cpu, &desc);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return z80_step(&cpu);
}

#define T(x) assert(x)

/* LD r,s; LD r,n */
void ld_r_sn() {
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

    T(7==step()); T(0x12==cpu.A);
    T(4==step()); T(0x12==cpu.B);
    T(4==step()); T(0x12==cpu.C);
    T(4==step()); T(0x12==cpu.D);
    T(4==step()); T(0x12==cpu.E);
    T(4==step()); T(0x12==cpu.H);
    T(4==step()); T(0x12==cpu.L);
    T(4==step()); T(0x12==cpu.A);
    T(7==step()); T(0x13==cpu.B);
    T(4==step()); T(0x13==cpu.B);
    T(4==step()); T(0x13==cpu.C);
    T(4==step()); T(0x13==cpu.D);
    T(4==step()); T(0x13==cpu.E);
    T(4==step()); T(0x13==cpu.H);
    T(4==step()); T(0x13==cpu.L);
    T(4==step()); T(0x13==cpu.A);
    T(7==step()); T(0x14==cpu.C);
    T(4==step()); T(0x14==cpu.B);
    T(4==step()); T(0x14==cpu.C);
    T(4==step()); T(0x14==cpu.D);
    T(4==step()); T(0x14==cpu.E);
    T(4==step()); T(0x14==cpu.H);
    T(4==step()); T(0x14==cpu.L);
    T(4==step()); T(0x14==cpu.A);
    T(7==step()); T(0x15==cpu.D);
    T(4==step()); T(0x15==cpu.B);
    T(4==step()); T(0x15==cpu.C);
    T(4==step()); T(0x15==cpu.D);
    T(4==step()); T(0x15==cpu.E);
    T(4==step()); T(0x15==cpu.H);
    T(4==step()); T(0x15==cpu.L);
    T(4==step()); T(0x15==cpu.A);
    T(7==step()); T(0x16==cpu.E);
    T(4==step()); T(0x16==cpu.B);
    T(4==step()); T(0x16==cpu.C);
    T(4==step()); T(0x16==cpu.D);
    T(4==step()); T(0x16==cpu.E);
    T(4==step()); T(0x16==cpu.H);
    T(4==step()); T(0x16==cpu.L);
    T(4==step()); T(0x16==cpu.A);
    T(7==step()); T(0x17==cpu.H);
    T(4==step()); T(0x17==cpu.B);
    T(4==step()); T(0x17==cpu.C);
    T(4==step()); T(0x17==cpu.D);
    T(4==step()); T(0x17==cpu.E);
    T(4==step()); T(0x17==cpu.H);
    T(4==step()); T(0x17==cpu.L);
    T(4==step()); T(0x17==cpu.A);
    T(7==step()); T(0x18==cpu.L);
    T(4==step()); T(0x18==cpu.B);
    T(4==step()); T(0x18==cpu.C);
    T(4==step()); T(0x18==cpu.D);
    T(4==step()); T(0x18==cpu.E);
    T(4==step()); T(0x18==cpu.H);
    T(4==step()); T(0x18==cpu.L);
    T(4==step()); T(0x18==cpu.A);
}


int main() {
    ld_r_sn();
    return 0;
}
