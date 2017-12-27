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

void w8(uint16_t addr, uint8_t data) {
    mem[addr] = data;
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

void LDA() {
    puts(">>> LDA");
    init();
    uint8_t prog[] = {
        // immediate
        0xA9, 0x00,         // LDA #$00
        0xA9, 0x01,         // LDA #$01
        0xA9, 0x00,         // LDA #$00
        0xA9, 0x80,         // LDA #$80

        // zero page
        0xA5, 0x02,         // LDA $02
        0xA5, 0x03,         // LDA $03
        0xA5, 0x80,         // LDA $80
        0xA5, 0xFF,         // LDA $FF

        // absolute
        0xAD, 0x00, 0x10,   // LDA $1000
        0xAD, 0xFF, 0xFF,   // LDA $FFFF
        0xAD, 0x21, 0x00,   // LDA $0021

        // zero page,X
        0xA2, 0x0F,         // LDX #$0F
        0xB5, 0x10,         // LDA $10,X    => 0x1F
        0xB5, 0xF8,         // LDA $FE,X    => 0x07
        0xB5, 0x78,         // LDA $78,X    => 0x87

        // absolute,X
        0xBD, 0xF1, 0x0F,   // LDA $0x0FF1,X    => 0x1000
        0xBD, 0xF0, 0xFF,   // LDA $0xFFF0,X    => 0xFFFF
        0xBD, 0x12, 0x00,   // LDA $0x0012,X    => 0x0021

        // absolute,Y
        0xA0, 0xF0,         // LDY #$F0
        0xB9, 0x10, 0x0F,   // LDA $0x0F10,Y    => 0x1000
        0xB9, 0x0F, 0xFF,   // LDA $0xFF0F,Y    => 0xFFFF
        0xB9, 0x31, 0xFF,   // LDA $0xFF31,Y    => 0x0021

        // indirect,X
        0xA1, 0xF0,         // LDA ($F0,X)  => 0xFF, second byte in 0x00 => 0x1234
        0xA1, 0x70,         // LDA ($70,X)  => 0x70 => 0x4321

        // indirect,Y
        0xB1, 0xFF,         // LDA ($FF),Y  => 0x1234+0xF0 => 0x1324
        0xB1, 0x7F,         // LDA ($7F),Y  => 0x4321+0xF0 => 0x4411
    };
    w8(0x0002, 0x01); w8(0x0003, 0x00); w8(0x0080, 0x80); w8(0x00FF, 0x03);
    w8(0x1000, 0x12); w8(0xFFFF, 0x34); w8(0x0021, 0x56);
    w8(0x001F, 0xAA); w8(0x0007, 0x33); w8(0x0087, 0x22);
    copy(0x0200, prog, sizeof(prog));

    // immediate
    T(2 == step()); T(cpu.A == 0x00); T(tf(M6502_ZF));
    T(2 == step()); T(cpu.A == 0x01); T(tf(0));
    T(2 == step()); T(cpu.A == 0x00); T(tf(M6502_ZF));
    T(2 == step()); T(cpu.A == 0x80); T(tf(M6502_NF));

    // zero page
    T(3 == step()); T(cpu.A == 0x01); T(tf(0));
    T(3 == step()); T(cpu.A == 0x00); T(tf(M6502_ZF));
    T(3 == step()); T(cpu.A == 0x80); T(tf(M6502_NF));
    T(3 == step()); T(cpu.A == 0x03); T(tf(0));

    // absolute
    T(4 == step()); T(cpu.A == 0x12); T(tf(0));
    T(4 == step()); T(cpu.A == 0x34); T(tf(0));
    T(4 == step()); T(cpu.A == 0x56); T(tf(0));

    // zero page,X
    T(2 == step()); T(cpu.X == 0x0F); T(tf(0));
    T(4 == step()); T(cpu.A == 0xAA); T(tf(M6502_NF));
    T(4 == step()); T(cpu.A == 0x33); T(tf(0));
    T(4 == step()); T(cpu.A == 0x22); T(tf(0));

    // absolute,X
    T(5 == step()); T(cpu.A == 0x12); T(tf(0));
    T(4 == step()); T(cpu.A == 0x34); T(tf(0));
    T(4 == step()); T(cpu.A == 0x56); T(tf(0));

    // absolute,Y
    T(2 == step()); T(cpu.Y == 0xF0); T(tf(0));
    T(5 == step()); T(cpu.A == 0x12); T(tf(0));
    T(4 == step()); T(cpu.A == 0x34); T(tf(0));
    T(5 == step()); T(cpu.A == 0x56); T(tf(0));

    // indirect,X
    w8(0x00FF, 0x34); w8(0x0000, 0x12); w16(0x007f, 0x4321);
    w8(0x1234, 0x89); w8(0x4321, 0x8A);
    T(6 == step()); T(cpu.A == 0x89); T(tf(M6502_NF));
    T(6 == step()); T(cpu.A == 0x8A); T(tf(M6502_NF));

    // indirect,Y (both 6 cycles because page boundary crossed)
    w8(0x1324, 0x98); w8(0x4411, 0xA8);
    T(6 == step()); T(cpu.A == 0x98); T(tf(M6502_NF));
    T(6 == step()); T(cpu.A == 0xA8); T(tf(M6502_NF));
}

int main() {
    INIT();
    RESET();
    LDA();
    printf("%d tests run ok.\n", num_tests);
    return 0;
}
