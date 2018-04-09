//------------------------------------------------------------------------------
//  z80-int.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80.h"
#include <stdio.h>

z80_t cpu;
uint8_t mem[1<<16] = { 0 };
bool reti_executed = false;

uint64_t tick(int num, uint64_t pins) {
    if (pins & Z80_MREQ) {
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, 0xFF);
        }
        else if (pins & Z80_WR) {
            /* request interrupt when a IORQ|WR happens */
            pins |= Z80_INT;
        }
        else if (pins & Z80_M1) {
            /* an interrupt acknowledge cycle, need to provide interrupt vector */
            Z80_SET_DATA(pins, 0xE0);
        }
    }
    if (pins & Z80_RETI) {
        /* reti was executed */
        reti_executed = true;
        pins &= ~Z80_RETI;
    }
    return pins;
}

void w16(uint16_t addr, uint16_t data) {
    mem[addr]   = (uint8_t) data;
    mem[addr+1] = (uint8_t) (data>>8);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return z80_exec(&cpu, 0);
}

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

int main() {
    z80_init(&cpu, tick);

    /* place the address 0x0200 of an interrupt service routine at 0x00E0 */
    w16(0x00E0, 0x0200);

    /* the main program load I with 0, and execute an OUT,
       which triggeres an interrupt, afterwards load some
       value into HL
    */
    uint8_t prog[] = {
        0x31, 0x00, 0x03,   /* LD SP,0x0300 */
        0xFB,               /* EI */
        0xED, 0x5E,         /* IM 2 */
        0xAF,               /* XOR A */
        0xED, 0x47,         /* LD I,A */
        0xD3, 0x01,         /* OUT (0x01),A -> this should request an interrupt */
        0x21, 0x33, 0x33,   /* LD HL,0x3333 */
    };
    copy(0x0100, prog, sizeof(prog));
    cpu.state.PC = 0x0100;

    /* the interrupt service routine */
    uint8_t int_prog[] = {
        0xFB,               /* EI */
        0x21, 0x11, 0x11,   /* LD HL,0x1111 */
        0xED, 0x4D,         /* RETI */
    };
    copy(0x0200, int_prog, sizeof(int_prog));

    T(10 == z80_exec(&cpu,0)); T(cpu.state.SP == 0x0300);   /* LD SP, 0x0300) */
    T(4 == z80_exec(&cpu,0)); T(cpu.state.IFF2 == false);                       /* EI */
    T(8 == z80_exec(&cpu,0)); T(cpu.state.IFF2 == true); T(cpu.state.IM == 2);        /* IM 2 */
    T(4 == z80_exec(&cpu,0)); T(cpu.state.A == 0);                              /* XOR A */
    T(9 == z80_exec(&cpu,0)); T(cpu.state.I == 0);                              /* LD I,A */
    T(29 == z80_exec(&cpu,0)); T(cpu.state.PC == 0x0200); T(cpu.state.IFF2 == false); T(cpu.state.SP == 0x02FE);    /* OUT (0x01),A */
    T(4 == z80_exec(&cpu,0)); T(cpu.state.IFF2 == false);                       /* EI */
    T(10 == z80_exec(&cpu,0)); T(cpu.state.HL == 0x1111); T(cpu.state.IFF2 == true);  /* LD HL,0x1111 */
    T(14 == z80_exec(&cpu,0)); T(cpu.state.PC == 0x010B); T(reti_executed);     /* RETI */
    z80_exec(&cpu,0); T(cpu.state.HL == 0x3333); /* LD HL,0x3333 */
}

