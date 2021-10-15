//------------------------------------------------------------------------------
//  z80x-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80x.h"

static z80_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16];
static uint8_t io[1<<16];

static void init(void) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
}

static void prefetch(uint16_t pc) {
    pins = Z80_M1|Z80_MREQ|Z80_RD;
    Z80_SET_ADDR(pins, pc);
    Z80_SET_DATA(pins, mem[pc]);
    cpu.pc = (pc + 1) & 0xFFFF;
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

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void test(void) {
    init();
    uint8_t prog[] = {
        0x3E, 0x11, // ld a,11h
        0x78,       // ld b,a
        0x41,       // ld c,b
        0x4A,       // ld d,c
    };
    copy(0x0100, prog, sizeof(prog));
    prefetch(0x0100);
    for (int i = 0; i < (7 + 3*4); i++) {
        tick();
    }
    assert(cpu.d == 0x11);
}

int main() {
    test();
}



