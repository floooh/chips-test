//------------------------------------------------------------------------------
//  m6502-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6502.h"
#include <stdio.h>

m6502_t cpu;
uint8_t mem[1<<16] = { 0 };

uint64_t tick(uint64_t pins) {
    const uint16_t addr = M6502_GET_ADDR(pins);
    if (pins & M6502_RW) {
        /* memory read */
        M6502_SET_DATA(pins, mem[addr]);
    }
    else {
        /* memory write */
        mem[addr] = M6502_GET_DATA(pins);
    }
    return pins;
}

void w16(uint16_t addr, uint16_t data) {
    mem[addr] = data & 0xFF;
    mem[(addr+1)&0xFFFF] = data>>8;
}

void init() {
    m6502_init(&cpu, tick);
    w16(0xFFFC, 0x0200);
    m6502_reset(&cpu);
}

void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

uint32_t step() {
    return m6502_exec(&cpu, 0);
}

bool tf(uint8_t expected) {
    /* XF is always set */
    expected |= M6502_XF;
    return (cpu.P & expected) == expected;
}

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void INIT() {
    puts(">>> INIT");
    init();
    T(0 == cpu.A);
    T(0 == cpu.X);
    T(0 == cpu.Y);
    T(0xFD == cpu.S);
    T(0x0200 == cpu.PC);
    T(tf(0));
}

void RESET() {
    puts(">>> RESET");
    init();
    w16(0xFFFC, 0x1234);
    m6502_reset(&cpu);
    T(0xFD == cpu.S);
    T(0x1234 == cpu.PC);
    T(tf(M6502_IF));
}

int main() {
    INIT();
    RESET();
    printf("%d tests run ok.\n", num_tests);
    return 0;
}
