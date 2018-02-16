//------------------------------------------------------------------------------
//  z80-wait.c
//  Test if Amstrad CPC-style wait state injection works.
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
uint32_t tick_count = 0;

uint64_t tick(int num, uint64_t pins) {
    // depending on the machine cycle type, the CPU samples the
    // wait pin either on the second, third or fourth machine cycle tick
    int wait_sample_tick = -1;
    if (pins & Z80_MREQ) {
        // a memory request or opcode fetch, wait is sampled on second clock tick
        wait_sample_tick = 1;
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_M1) {
            // an interrupt acknowledge cycle, wait is sampled on fourth clock tick
            wait_sample_tick = 3;
        }
        else {
            // an IO cycle, wait is sampled on third clock tick
            wait_sample_tick = 2;
        }
    }
    bool wait = false;
    uint32_t wait_cycles = 0;
    for (int i = 0; i<num; i++) {
        do {
            // CPC gate array sets the wait pin for 3 out of 4 clock ticks
            bool wait_pin = (tick_count++ & 3) != 0;
            wait = (wait_pin && (wait || (i == wait_sample_tick)));
            if (wait) {
                wait_cycles++;
            }
        }
        while (wait);
    }
    assert(wait_cycles <= 7);
    Z80_SET_WAIT(pins, wait_cycles);
    // memory and IO requests
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
            // FIXME: IO read
        }
        else if (pins & Z80_WR) {
            // FIXME: IO write
        }
    }
    return pins;
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return z80_exec(&cpu, 0);
}

void log_ticks(uint32_t ticks) {
    printf("%d\n", ticks);
}

int main() {
    z80_init(&cpu, tick);

    uint8_t prog[] = {
        0x00,               // NOP
        0x00,               // NOP
        0x00,               // NOP
        0x00,               // NOP
        0x21, 0x00, 0x10,   // LD HL,0x1000
        0x00,               // NOP
        0x21, 0x00, 0x20,   // LD HL,0x2000
        0x21, 0x00, 0x30,   // LD HL,0x3000
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
    copy(0x0000, prog, sizeof(prog));

    for (int i = 0; i < 20; i++) {
        log_ticks(step());
    }
    return 0;
}