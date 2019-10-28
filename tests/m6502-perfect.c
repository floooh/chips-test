//------------------------------------------------------------------------------
//  m6502-perfect.c
//
//  Test the cycle-stepped m6502 emulator against the transistor-level
//  6502 simulation from https://github.com/mist64/perfect6502
//
//  What's checked:
//  - after each tick: compare the state of the address-, data-, SYNC- and
//    RW-pins between the two emulators
//  - after each instruction: check if the expected number of ticks have
//    passed, and the registers and flags bits are in the expected state
//
//  The perfect6502 simulator is stepped in half-cycles, and runs one half-cycle
//  ahead of the cycle-stepped m6502 emulator (so that results computed
//  in the previous instruction - which overlaps with the opcode fetch of the
//  next instruction - are available in registers for testing).
//------------------------------------------------------------------------------
#include "perfect6502/types.h"
#include "perfect6502/netlist_sim.h"
#include "perfect6502/perfect6502.h"
#include "chips/m6502x.h"
#include "utest.h"
#include <assert.h>

#define T(b) ASSERT_TRUE(b)
#define STEP(x) step_until_sync(x)

// perfect6502's CPU state
static void* p6502_state;

// our own emulator state and memory
static m6502x_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16] = { 0 };

// copy a range of bytes into both memories
static void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    // our own memory
    memcpy(&mem[addr], bytes, num);
    // perfect6502's memory
    memcpy(&memory[addr], bytes, num);
}

// write a byte into both emulator's memories
static void w8(uint16_t addr, uint8_t data) {
    // our own memory
    mem[addr] = data;
    // perfect6502's memory
    memory[addr] = data;
}

// read a byte from both memories, and check against expected value
static bool r8(uint16_t addr, uint8_t expected) {
    return (mem[addr] == expected) && (memory[addr] == expected);
}

// write a 16-bit value into both memories
static void w16(uint16_t addr, uint16_t data) {
    // our own memory
    mem[addr] = data & 0xFF;
    mem[(addr+1)&0xFFFF] = data>>8;
    // perfect6502's memory
    memory[addr] = data & 0xFF;
    memory[(addr+1)&0xFFFF] = data>>8;
}

// read 16-bit value from both memories and check against expected value
static bool r16(uint16_t addr, uint16_t expected) {
    uint8_t l, h;
    // our own memory
    l = mem[addr];
    h = mem[(addr+1)&0xFFFF];
    uint16_t v16_0 = (h<<8)|l;
    // perfect6502's memory
    l = memory[addr];
    h = memory[(addr+1)&0xFFFF];
    uint16_t v16_1 = (h<<8)|l;
    return (v16_0 == expected) && (v16_1 == expected);
}

// initialize both emulators
static void init() {
    memset(mem, 0, sizeof(mem));
    memset(memory, 0, sizeof(memory));
    m6502x_init(&cpu, &(m6502x_desc_t){0});
    if (p6502_state) {
        destroyChip(p6502_state);
        p6502_state = 0;
    }
    p6502_state = initAndResetChip();
}

// reset both emulators through their reset sequence to a starting address
static void prefetch(uint16_t start_addr) {
    // write starting address to the 6502's reset vector location
    w16(0xFFFC, start_addr);

    // FIXME: implement proper reset start sequence!
    pins = 0;
    cpu.IR = mem[start_addr]<<3;
    cpu.PC = start_addr+1;
    cpu.S = 0xBD;

    // run through the perfect6502 9-cycle reset sequence
    // here, the SP starts as 0xC0 and is reduced to 0xBD after the reset routine
    for (int i = 0; i < 9; i++) {
        step(p6502_state);
        step(p6502_state);
    }
    assert(readPC(p6502_state) == start_addr);
    // run one half-tick ahead into the next instruction
    step(p6502_state);
}

// perform memory access for our own emulator
static uint64_t mem_access(uint64_t pins) {
    const uint16_t addr = M6502X_GET_ADDR(pins);
    if (pins & M6502X_RW) {
        /* read */
        uint8_t val = mem[addr];
        M6502X_SET_DATA(pins, val);
    }
    else {
        /* write */
        uint8_t val = M6502X_GET_DATA(pins);
        mem[addr] = val;
    }
    return pins;
}

// step both emulators through the current instruction,
// compare the relevant pin state after each tick, and finally
// check for expected number of ticks
static bool step_until_sync(uint32_t expected_ticks) {
    uint32_t tick = 0;
    do {
        // step our own emu
        pins = m6502x_tick(&cpu, pins);
        pins = mem_access(pins);
        // step perfect6502 simulation (in half-steps)
        if (tick > 0) {
            // skip the first half tick which was executed in the last invocation
            step(p6502_state);
        }
        step(p6502_state);
        // check whether both emulators agree on the observable pin state after each tick
        uint16_t m6502_addr = M6502X_GET_ADDR(pins);
        uint16_t p6502_addr  = readAddressBus(p6502_state);
        if (m6502_addr != p6502_addr) {
            return false;
        }
        uint8_t m6502_data = M6502X_GET_DATA(pins);
        uint8_t p6502_data  = readDataBus(p6502_state);
        if (m6502_data != p6502_data) {
            return false;
        }
        bool m6502_sync = (pins & M6502X_SYNC);
        bool p6502_sync = isNodeHigh(p6502_state, 539); // 539 is SYNC pin
        if (m6502_sync != p6502_sync) {
            return false;
        }
        bool m6502_rw = (pins & M6502X_RW);
        bool p6502_rw = readRW(p6502_state);
        if (m6502_rw != p6502_rw) {
            return false;
        }
        tick++;
    } while (!isNodeHigh(p6502_state, 539)); // 539 is SYNC pin, next instruction about to begin
    // run one half tick into next instruction so that overlapped operations can finish
    step(p6502_state);
    return (tick == expected_ticks);
}

// check the flag bits and registers in both emulators against expected value
static bool tf(uint8_t expected) {
    uint8_t p6502_p = readP(p6502_state) & ~(M6502X_XF|M6502X_IF|M6502X_BF);
    uint8_t m6502_p = cpu.P & ~(M6502X_XF|M6502X_IF|M6502X_BF);
    return (p6502_p == expected) && (m6502_p == expected);
}

static bool ra(uint8_t expected) {
    uint8_t p6502_a = readA(p6502_state);
    uint8_t m6502_a = cpu.A;
    return (p6502_a == expected) && (m6502_a == expected);
}

static bool rx(uint8_t expected) {
    uint8_t p6502_x = readX(p6502_state);
    uint8_t m6502_x = cpu.X;
    return (p6502_x == expected) && (m6502_x == expected);
}

static bool ry(uint8_t expected) {
    uint8_t p6502_y = readY(p6502_state);
    uint8_t m6502_y = cpu.Y;
    return (p6502_y == expected) && (m6502_y == expected);
}

static bool rs(uint8_t expected) {
    uint8_t p6502_s = readSP(p6502_state);
    uint8_t m6502_s = cpu.S;
    return (p6502_s == expected) && (m6502_s == expected);
}

static bool rpc(uint16_t expected) {
    uint16_t p6502_pc = readPC(p6502_state);
    uint16_t m6502_pc = cpu.PC;
    return (p6502_pc == expected) && (m6502_pc == expected);
}

/*=== TESTS START HERE =======================================================*/
UTEST(m6502_perfect, LDA) {
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
    prefetch(0x0200);

    // immediate
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0x01)); T(tf(0));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0x80)); T(tf(M6502X_NF));

    // zero page
    T(STEP(3)); T(ra(0x01)); T(tf(0));
    T(STEP(3)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(3)); T(ra(0x80)); T(tf(M6502X_NF));
    T(STEP(3)); T(ra(0x03)); T(tf(0));

    // absolute
    T(STEP(4)); T(ra(0x12)); T(tf(0));
    T(STEP(4)); T(ra(0x34)); T(tf(0));
    T(STEP(4)); T(ra(0x56)); T(tf(0));

    // zero page,X
    T(STEP(2)); T(rx(0x0F)); T(tf(0));
    T(STEP(4)); T(ra(0xAA)); T(tf(M6502X_NF));
    T(STEP(4)); T(ra(0x33)); T(tf(0));
    T(STEP(4)); T(ra(0x22)); T(tf(0));

    // absolute,X
    T(STEP(5)); T(ra(0x12)); T(tf(0));
    T(STEP(4)); T(ra(0x34)); T(tf(0));
    T(STEP(4)); T(ra(0x56)); T(tf(0));

    // absolute,Y
    T(STEP(2)); T(ry(0xF0)); T(tf(M6502X_NF));
    T(STEP(5)); T(ra(0x12)); T(tf(0));
    T(STEP(4)); T(ra(0x34)); T(tf(0));
    T(STEP(5)); T(ra(0x56)); T(tf(0));

    // indirect,X
    w8(0x00FF, 0x34); w8(0x0000, 0x12); w16(0x007f, 0x4321);
    w8(0x1234, 0x89); w8(0x4321, 0x8A);
    T(STEP(6)); T(ra(0x89)); T(tf(M6502X_NF));
    T(STEP(6)); T(ra(0x8A)); T(tf(M6502X_NF));

    // indirect,Y (both 6 cycles because page boundary crossed)
    w8(0x1324, 0x98); w8(0x4411, 0xA8);
    T(STEP(6)); T(ra(0x98)); T(tf(M6502X_NF));
    T(STEP(6)); T(ra(0xA8)); T(tf(M6502X_NF));
}

UTEST(m6502_perfect, LDX) {
    init();
    uint8_t prog[] = {
        // immediate
        0xA2, 0x00,         // LDX #$00
        0xA2, 0x01,         // LDX #$01
        0xA2, 0x00,         // LDX #$00
        0xA2, 0x80,         // LDX #$80

        // zero page
        0xA6, 0x02,         // LDX $02
        0xA6, 0x03,         // LDX $03
        0xA6, 0x80,         // LDX $80
        0xA6, 0xFF,         // LDX $FF

        // absolute
        0xAE, 0x00, 0x10,   // LDX $1000
        0xAE, 0xFF, 0xFF,   // LDX $FFFF
        0xAE, 0x21, 0x00,   // LDX $0021

        // zero page,Y
        0xA0, 0x0F,         // LDY #$0F
        0xB6, 0x10,         // LDX $10,Y    => 0x1F
        0xB6, 0xF8,         // LDX $FE,Y    => 0x07
        0xB6, 0x78,         // LDX $78,Y    => 0x87

        // absolute,Y
        0xA0, 0xF0,         // LDY #$F0
        0xBE, 0x10, 0x0F,   // LDX $0F10,Y    => 0x1000
        0xBE, 0x0F, 0xFF,   // LDX $FF0F,Y    => 0xFFFF
        0xBE, 0x31, 0xFF,   // LDX $FF31,Y    => 0x0021
    };
    w8(0x0002, 0x01); w8(0x0003, 0x00); w8(0x0080, 0x80); w8(0x00FF, 0x03);
    w8(0x1000, 0x12); w8(0xFFFF, 0x34); w8(0x0021, 0x56);
    w8(0x001F, 0xAA); w8(0x0007, 0x33); w8(0x0087, 0x22);
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    // immediate
    T(STEP(2)); T(rx(0x00)); (tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x01)); (tf(0));
    T(STEP(2)); T(rx(0x00)); (tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x80)); (tf(M6502X_NF));

    // zero page
    T(STEP(3)); T(rx(0x01)); T(tf(0));
    T(STEP(3)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(3)); T(rx(0x80)); T(tf(M6502X_NF));
    T(STEP(3)); T(rx(0x03)); T(tf(0));

    // absolute
    T(STEP(4)); T(rx(0x12)); T(tf(0));
    T(STEP(4)); T(rx(0x34)); T(tf(0));
    T(STEP(4)); T(rx(0x56)); T(tf(0));

    // zero page,Y
    T(STEP(2)); T(ry(0x0F)); T(tf(0));
    T(STEP(4)); T(rx(0xAA)); T(tf(M6502X_NF));
    T(STEP(4)); T(rx(0x33)); T(tf(0));
    T(STEP(4)); T(rx(0x22)); T(tf(0));

    // absolute,X
    T(STEP(2)); T(ry(0xF0)); T(tf(M6502X_NF));
    T(STEP(5)); T(rx(0x12)); T(tf(0));
    T(STEP(4)); T(rx(0x34)); T(tf(0));
    T(STEP(5)); T(rx(0x56)); T(tf(0));
}

UTEST(m6502_perfect, LDY) {
    init();
    uint8_t prog[] = {
        // immediate
        0xA0, 0x00,         // LDY #$00
        0xA0, 0x01,         // LDY #$01
        0xA0, 0x00,         // LDY #$00
        0xA0, 0x80,         // LDY #$80

        // zero page
        0xA4, 0x02,         // LDY $02
        0xA4, 0x03,         // LDY $03
        0xA4, 0x80,         // LDY $80
        0xA4, 0xFF,         // LDY $FF

        // absolute
        0xAC, 0x00, 0x10,   // LDY $1000
        0xAC, 0xFF, 0xFF,   // LDY $FFFF
        0xAC, 0x21, 0x00,   // LDY $0021

        // zero page,X
        0xA2, 0x0F,         // LDX #$0F
        0xB4, 0x10,         // LDY $10,X    => 0x1F
        0xB4, 0xF8,         // LDY $FE,X    => 0x07
        0xB4, 0x78,         // LDY $78,X    => 0x87

        // absolute,X
        0xA2, 0xF0,         // LDX #$F0
        0xBC, 0x10, 0x0F,   // LDY $0F10,X    => 0x1000
        0xBC, 0x0F, 0xFF,   // LDY $FF0F,X    => 0xFFFF
        0xBC, 0x31, 0xFF,   // LDY $FF31,X    => 0x0021
    };
    w8(0x0002, 0x01); w8(0x0003, 0x00); w8(0x0080, 0x80); w8(0x00FF, 0x03);
    w8(0x1000, 0x12); w8(0xFFFF, 0x34); w8(0x0021, 0x56);
    w8(0x001F, 0xAA); w8(0x0007, 0x33); w8(0x0087, 0x22);
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    // immediate
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0x80)); T(tf(M6502X_NF));

    // zero page
    T(STEP(3)); T(ry(0x01)); T(tf(0));
    T(STEP(3)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(3)); T(ry(0x80)); T(tf(M6502X_NF));
    T(STEP(3)); T(ry(0x03)); T(tf(0));

    // absolute
    T(STEP(4)); T(ry(0x12)); T(tf(0));
    T(STEP(4)); T(ry(0x34)); T(tf(0));
    T(STEP(4)); T(ry(0x56)); T(tf(0));

    // zero page,Y
    T(STEP(2)); T(rx(0x0F)); T(tf(0));
    T(STEP(4)); T(ry(0xAA)); T(tf(M6502X_NF));
    T(STEP(4)); T(ry(0x33)); T(tf(0));
    T(STEP(4)); T(ry(0x22)); T(tf(0));

    // absolute,X
    T(STEP(2)); T(rx(0xF0)); T(tf(M6502X_NF));
    T(STEP(5)); T(ry(0x12)); T(tf(0));
    T(STEP(4)); T(ry(0x34)); T(tf(0));
    T(STEP(5)); T(ry(0x56)); T(tf(0));
}

UTEST(m6502_perfect, STA) {
    init();
    uint8_t prog[] = {
        0xA9, 0x23,             // LDA #$23
        0xA2, 0x10,             // LDX #$10
        0xA0, 0xC0,             // LDY #$C0
        0x85, 0x10,             // STA $10
        0x8D, 0x34, 0x12,       // STA $1234
        0x95, 0x10,             // STA $10,X
        0x9D, 0x00, 0x20,       // STA $2000,X
        0x99, 0x00, 0x20,       // STA $2000,Y
        0x81, 0x10,             // STA ($10,X)
        0x91, 0x20,             // STA ($20),Y
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x23));
    T(STEP(2)); T(rx(0x10));
    T(STEP(2)); T(ry(0xC0));
    T(STEP(3)); T(r8(0x0010, 0x23));
    T(STEP(4)); T(r8(0x1234, 0x23));
    T(STEP(4)); T(r8(0x0020, 0x23));
    T(STEP(5)); T(r8(0x2010, 0x23));
    T(STEP(5)); T(r8(0x20C0, 0x23));
    w16(0x0020, 0x4321);
    T(STEP(6)); T(r8(0x4321, 0x23));
    T(STEP(6)); T(r8(0x43E1, 0x23));
}

UTEST(m6502_perfect, STX) {
    init();
    uint8_t prog[] = {
        0xA2, 0x23,             // LDX #$23
        0xA0, 0x10,             // LDY #$10

        0x86, 0x10,             // STX $10
        0x8E, 0x34, 0x12,       // STX $1234
        0x96, 0x10,             // STX $10,Y
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(rx(0x23));
    T(STEP(2)); T(ry(0x10));
    T(STEP(3)); T(r8(0x0010, 0x23));
    T(STEP(4)); T(r8(0x1234, 0x23));
    T(STEP(4)); T(r8(0x0020, 0x23));
}

UTEST(m6502_perfect, STY) {
    init();
    uint8_t prog[] = {
        0xA0, 0x23,             // LDY #$23
        0xA2, 0x10,             // LDX #$10

        0x84, 0x10,             // STX $10
        0x8C, 0x34, 0x12,       // STX $1234
        0x94, 0x10,             // STX $10,Y
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ry(0x23));
    T(STEP(2)); T(rx(0x10));
    T(STEP(3)); T(r8(0x0010, 0x23));
    T(STEP(4)); T(r8(0x1234, 0x23));
    T(STEP(4)); T(r8(0x0020, 0x23));
}

UTEST(m6502_perfect, TAX_TXA) {
    init();
    uint8_t prog[] = {
        0xA9, 0x00,     // LDA #$00
        0xA2, 0x10,     // LDX #$10
        0xAA,           // TAX
        0xA9, 0xF0,     // LDA #$F0
        0x8A,           // TXA
        0xA9, 0xF0,     // LDA #$F0
        0xA2, 0x00,     // LDX #$00
        0xAA,           // TAX
        0xA9, 0x01,     // LDA #$01
        0x8A,           // TXA
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x10)); T(tf(0));
    T(STEP(2)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x01)); T(tf(0));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
}

UTEST(m6502_perfect, TAY_TYA) {
    init();
    uint8_t prog[] = {
        0xA9, 0x00,     // LDA #$00
        0xA0, 0x10,     // LDY #$10
        0xA8,           // TAY
        0xA9, 0xF0,     // LDA #$F0
        0x98,           // TYA
        0xA9, 0xF0,     // LDA #$F0
        0xA0, 0x00,     // LDY #$00
        0xA8,           // TAY
        0xA9, 0x01,     // LDA #$01
        0x98,           // TYA
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0x10)); T(tf(0));
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0xF0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x01)); T(tf(0));
    T(STEP(2)); T(ra(0xF0)); T(tf(M6502X_NF));
}

UTEST(m6502_perfect, DEX_INX_DEY_INY) {
    init();
    uint8_t prog[] = {
        0xA2, 0x01,     // LDX #$01
        0xCA,           // DEX
        0xCA,           // DEX
        0xE8,           // INX
        0xE8,           // INX
        0xA0, 0x01,     // LDY #$01
        0x88,           // DEY
        0x88,           // DEY
        0xC8,           // INY
        0xC8,           // INY
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(rx(0x01)); T(tf(0));
    T(STEP(2)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0xFF)); T(tf(M6502X_NF));
    T(STEP(2)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0xFF)); T(tf(M6502X_NF));
    T(STEP(2)); T(ry(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(ry(0x01)); T(tf(0));
}

UTEST(m6502_perfect, TXS_TSX) {
    init();
    uint8_t prog[] = {
        0xA2, 0xAA,     // LDX #$AA
        0xA9, 0x00,     // LDA #$00
        0x9A,           // TXS
        0xAA,           // TAX
        0xBA,           // TSX
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(rx(0xAA)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rs(0xAA)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0xAA)); T(tf(M6502X_NF));
}

UTEST(m6502_perfect, ORA) {
    init();
    uint8_t prog[] = {
        0xA9, 0x00,         // LDA #$00
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x09, 0x00,         // ORA #$00
        0x05, 0x10,         // ORA $10
        0x15, 0x10,         // ORA $10,X
        0x0d, 0x00, 0x10,   // ORA $1000
        0x1d, 0x00, 0x10,   // ORA $1000,X
        0x19, 0x00, 0x10,   // ORA $1000,Y
        0x01, 0x22,         // ORA ($22,X)
        0x11, 0x20,         // ORA ($20),Y
    };
    copy(0x0200, prog, sizeof(prog));
    w16(0x0020, 0x1002);
    w16(0x0023, 0x1003);
    w8(0x0010, (1<<0));
    w8(0x0011, (1<<1));
    w8(0x1000, (1<<2));
    w8(0x1001, (1<<3));
    w8(0x1002, (1<<4));
    w8(0x1003, (1<<5));
    w8(0x1004, (1<<6));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(2)); T(rx(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x02)); T(tf(0));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(3)); T(ra(0x01)); T(tf(0));
    T(STEP(4)); T(ra(0x03)); T(tf(0));
    T(STEP(4)); T(ra(0x07)); T(tf(0));
    T(STEP(4)); T(ra(0x0F)); T(tf(0));
    T(STEP(4)); T(ra(0x1F)); T(tf(0));
    T(STEP(6)); T(ra(0x3F)); T(tf(0));
    T(STEP(5)); T(ra(0x7F)); T(tf(0));
}

UTEST(m6502_perfect, AND) {
    init();
    uint8_t prog[] = {
        0xA9, 0xFF,         // LDA #$FF
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x29, 0xFF,         // AND #$FF
        0x25, 0x10,         // AND $10
        0x35, 0x10,         // AND $10,X
        0x2d, 0x00, 0x10,   // AND $1000
        0x3d, 0x00, 0x10,   // AND $1000,X
        0x39, 0x00, 0x10,   // AND $1000,Y
        0x21, 0x22,         // AND ($22,X)
        0x31, 0x20,         // AND ($20),Y
    };
    copy(0x0200, prog, sizeof(prog));
    w16(0x0020, 0x1002);
    w16(0x0023, 0x1003);
    w8(0x0010, 0x7F);
    w8(0x0011, 0x3F);
    w8(0x1000, 0x1F);
    w8(0x1001, 0x0F);
    w8(0x1002, 0x07);
    w8(0x1003, 0x03);
    w8(0x1004, 0x01);
    prefetch(0x0200);

    T(STEP(2)); T(ra(0xFF)); T(tf(M6502X_NF));
    T(STEP(2)); T(rx(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x02)); T(tf(0));
    T(STEP(2)); T(ra(0xFF)); T(tf(M6502X_NF));
    T(STEP(3)); T(ra(0x7F)); T(tf(0));
    T(STEP(4)); T(ra(0x3F)); T(tf(0));
    T(STEP(4)); T(ra(0x1F)); T(tf(0));
    T(STEP(4)); T(ra(0x0F)); T(tf(0));
    T(STEP(4)); T(ra(0x07)); T(tf(0));
    T(STEP(6)); T(ra(0x03)); T(tf(0));
    T(STEP(5)); T(ra(0x01)); T(tf(0));
}

UTEST(m6502_perfect, EOR) {
    init();
    uint8_t prog[] = {
        0xA9, 0xFF,         // LDA #$FF
        0xA2, 0x01,         // LDX #$01
        0xA0, 0x02,         // LDY #$02
        0x49, 0xFF,         // EOR #$FF
        0x45, 0x10,         // EOR $10
        0x55, 0x10,         // EOR $10,X
        0x4d, 0x00, 0x10,   // EOR $1000
        0x5d, 0x00, 0x10,   // EOR $1000,X
        0x59, 0x00, 0x10,   // EOR $1000,Y
        0x41, 0x22,         // EOR ($22,X)
        0x51, 0x20,         // EOR ($20),Y
    };
    copy(0x0200, prog, sizeof(prog));
    w16(0x0020, 0x1002);
    w16(0x0023, 0x1003);
    w8(0x0010, 0x7F);
    w8(0x0011, 0x3F);
    w8(0x1000, 0x1F);
    w8(0x1001, 0x0F);
    w8(0x1002, 0x07);
    w8(0x1003, 0x03);
    w8(0x1004, 0x01);
    prefetch(0x0200);

    T(STEP(2)); T(ra(0xFF)); T(tf(M6502X_NF));
    T(STEP(2)); T(rx(0x01)); T(tf(0));
    T(STEP(2)); T(ry(0x02)); T(tf(0));
    T(STEP(2)); T(ra(0x00)); T(tf(M6502X_ZF));
    T(STEP(3)); T(ra(0x7F)); T(tf(0));
    T(STEP(4)); T(ra(0x40)); T(tf(0));
    T(STEP(4)); T(ra(0x5F)); T(tf(0));
    T(STEP(4)); T(ra(0x50)); T(tf(0));
    T(STEP(4)); T(ra(0x57)); T(tf(0));
    T(STEP(6)); T(ra(0x54)); T(tf(0));
    T(STEP(5)); T(ra(0x55)); T(tf(0));
}

UTEST(m6502_perfect, NOP) {
    init();
    uint8_t prog[] = {
        0xEA,       // NOP
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);
    T(STEP(2));
}

UTEST(m6502_perfect, PHA_PLA_PHP_PLP) {
    init();
    uint8_t prog[] = {
        0xA9, 0x23,     // LDA #$23
        0x48,           // PHA
        0xA9, 0x32,     // LDA #$32
        0x68,           // PLA
        0x08,           // PHP
        0xA9, 0x00,     // LDA #$00
        0x28,           // PLP
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x23)); T(rs(0xBD));
    T(STEP(3)); T(rs(0xBC)); T(r8(0x01BD, 0x23));
    T(STEP(2)); T(ra(0x32));
    T(STEP(4)); T(ra(0x23)); T(rs(0xBD)); T(tf(0));
    T(STEP(3)); T(rs(0xBC)); T(r8(0x01BD, M6502X_XF|M6502X_IF|M6502X_BF));
    T(STEP(2)); T(tf(M6502X_ZF));
    T(STEP(4)); T(rs(0xBD)); T(tf(0));
}

UTEST(m6502_perfect, CLC_SEC_CLI_SEI_CLV_CLD_SED) {
    init();
    uint8_t prog[] = {
        0xB8,       // CLV
        0x78,       // SEI
        0x58,       // CLI
        0x38,       // SEC
        0x18,       // CLC
        0xF8,       // SED
        0xD8,       // CLD
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(tf(M6502X_ZF));
    T(STEP(2)); //T(tf(M6502X_ZF|M6502X_IF));   // FIXME: interrupt bit is ignored in tf()
    T(STEP(2)); T(tf(M6502X_ZF));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(2)); T(tf(M6502X_ZF));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_DF));
    T(STEP(2)); T(tf(M6502X_ZF));
}

UTEST(m6502_perfect, INC_DEC) {
    init();
    uint8_t prog[] = {
        0xA2, 0x10,         // LDX #$10
        0xE6, 0x33,         // INC $33
        0xF6, 0x33,         // INC $33,X
        0xEE, 0x00, 0x10,   // INC $1000
        0xFE, 0x00, 0x10,   // INC $1000,X
        0xC6, 0x33,         // DEC $33
        0xD6, 0x33,         // DEC $33,X
        0xCE, 0x00, 0x10,   // DEC $1000
        0xDE, 0x00, 0x10,   // DEC $1000,X
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(rx(0x10));
    T(STEP(5)); T(r8(0x0033,1)); T(tf(0));
    T(STEP(6)); T(r8(0x0043,1)); T(tf(0));
    T(STEP(6)); T(r8(0x1000,1)); T(tf(0));
    T(STEP(7)); T(r8(0x1010,1)); T(tf(0));
    T(STEP(5)); T(r8(0x0033,0)); T(tf(M6502X_ZF));
    T(STEP(6)); T(r8(0x0043,0)); T(tf(M6502X_ZF));
    T(STEP(6)); T(r8(0x1000,0)); T(tf(M6502X_ZF));
    T(STEP(7)); T(r8(0x1010,0)); T(tf(M6502X_ZF));
}

UTEST(m6502_perfect, ADC_SBC) {
    init();
    uint8_t prog[] = {
        0xA9, 0x01,         // LDA #$01
        0x85, 0x10,         // STA $10
        0x8D, 0x00, 0x10,   // STA $1000
        0xA9, 0xFC,         // LDA #$FC
        0xA2, 0x08,         // LDX #$08
        0xA0, 0x04,         // LDY #$04
        0x18,               // CLC
        0x69, 0x01,         // ADC #$01
        0x65, 0x10,         // ADC $10
        0x75, 0x08,         // ADC $8,X
        0x6D, 0x00, 0x10,   // ADC $1000
        0x7D, 0xF8, 0x0F,   // ADC $0FF8,X
        0x79, 0xFC, 0x0F,   // ADC $0FFC,Y
        // FIXME: ADC (zp,X) and ADC (zp),X
        0xF9, 0xFC, 0x0F,   // SBC $0FFC,Y
        0xFD, 0xF8, 0x0F,   // SBC $0FF8,X
        0xED, 0x00, 0x10,   // SBC $1000
        0xF5, 0x08,         // SBC $8,X
        0xE5, 0x10,         // SBC $10
        0xE9, 0x01,         // SBC #$10
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x01));
    T(STEP(3)); T(r8(0x0010, 0x01));
    T(STEP(4)); T(r8(0x1000, 0x01));
    T(STEP(2)); T(ra(0xFC));
    T(STEP(2)); T(rx(0x08));
    T(STEP(2)); T(ry(0x04))
    T(STEP(2));  // CLC
    // ADC
    T(STEP(2)); T(ra(0xFD)); T(tf(M6502X_NF));
    T(STEP(3)); T(ra(0xFE)); T(tf(M6502X_NF));
    T(STEP(4)); T(ra(0xFF)); T(tf(M6502X_NF));
    T(STEP(4)); T(ra(0x00)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(5)); T(ra(0x02)); T(tf(0));
    T(STEP(5)); T(ra(0x03)); T(tf(0));
    // SBC
    T(STEP(5)); T(ra(0x01)); T(tf(M6502X_CF));
    T(STEP(5)); T(ra(0x00)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(4)); T(ra(0xFF)); T(tf(M6502X_NF));
    T(STEP(4)); T(ra(0xFD)); T(tf(M6502X_NF|M6502X_CF));
    T(STEP(3)); T(ra(0xFC)); T(tf(M6502X_NF|M6502X_CF));
    T(STEP(2)); T(ra(0xFB)); T(tf(M6502X_NF|M6502X_CF));
}

UTEST(m6502_perfect, CMP_CPX_CPY) {
    init();
    // FIXME: non-immediate addressing modes
    uint8_t prog[] = {
        0xA9, 0x01,     // LDA #$01
        0xA2, 0x02,     // LDX #$02
        0xA0, 0x03,     // LDY #$03
        0xC9, 0x00,     // CMP #$00
        0xC9, 0x01,     // CMP #$01
        0xC9, 0x02,     // CMP #$02
        0xE0, 0x01,     // CPX #$01
        0xE0, 0x02,     // CPX #$02
        0xE0, 0x03,     // CPX #$03
        0xC0, 0x02,     // CPY #$02
        0xC0, 0x02,     // CPY #$03
        0xC0, 0x03,     // CPY #$04
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x01));
    T(STEP(2)); T(rx(0x02));
    T(STEP(2)); T(ry(0x03));
    T(STEP(2)); T(tf(M6502X_CF));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(2)); T(tf(M6502X_NF));
    T(STEP(2)); T(tf(M6502X_CF));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(2)); T(tf(M6502X_NF));
}

UTEST(m6502_perfect, ASL) {
    init();
    // FIXME: more addressing modes
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x06, 0x10,     // ASL $10
        0x16, 0x0F,     // ASL $0F,X
        0x0A,           // ASL
        0x0A,           // ASL
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x81));
    T(STEP(2)); T(rx(0x01));
    T(STEP(3)); T(r8(0x0010, 0x81));
    T(STEP(5)); T(r8(0x0010, 0x02)); T(tf(M6502X_CF));
    T(STEP(6)); T(r8(0x0010, 0x04)); T(tf(0));
    T(STEP(2)); T(ra(0x02)); T(tf(M6502X_CF));
    T(STEP(2)); T(ra(0x04)); T(tf(0));
}

UTEST(m6502_perfect, LSR) {
    init();
    // FIXME: more addressing modes
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x46, 0x10,     // LSR $10
        0x56, 0x0F,     // LSR $0F,X
        0x4A,           // LSR
        0x4A,           // LSR
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x81));
    T(STEP(2)); T(rx(0x01));
    T(STEP(3)); T(r8(0x0010, 0x81));
    T(STEP(5)); T(r8(0x0010, 0x40)); T(tf(M6502X_CF));
    T(STEP(6)); T(r8(0x0010, 0x20)); T(tf(0));
    T(STEP(2)); T(ra(0x40)); T(tf(M6502X_CF));
    T(STEP(2)); T(ra(0x20)); T(tf(0));
}

UTEST(m6502_prefetch, ROR_ROL) {
    init();
    // FIXME: more adressing modes
    uint8_t prog[] = {
        0xA9, 0x81,     // LDA #$81
        0xA2, 0x01,     // LDX #$01
        0x85, 0x10,     // STA $10
        0x26, 0x10,     // ROL $10
        0x36, 0x0F,     // ROL $0F,X
        0x76, 0x0F,     // ROR $0F,X
        0x66, 0x10,     // ROR $10
        0x6A,           // ROR
        0x6A,           // ROR
        0x2A,           // ROL
        0x2A,           // ROL
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x81));
    T(STEP(2)); T(rx(0x01));
    T(STEP(3)); T(r8(0x0010, 0x81));
    T(STEP(5)); T(r8(0x0010, 0x02)); T(tf(M6502X_CF));
    T(STEP(6)); T(r8(0x0010, 0x05)); T(tf(0));
    T(STEP(6)); T(r8(0x0010, 0x02)); T(tf(M6502X_CF));
    T(STEP(5)); T(r8(0x0010, 0x81)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x40)); T(tf(M6502X_CF));
    T(STEP(2)); T(ra(0xA0)); T(tf(M6502X_NF));
    T(STEP(2)); T(ra(0x40)); T(tf(M6502X_CF));
    T(STEP(2)); T(ra(0x81)); T(tf(M6502X_NF));
}


UTEST(m6502_perfect, BIT) {
    init();
    uint8_t prog[] = {
        0xA9, 0x00,         // LDA #$00
        0x85, 0x1F,         // STA $1F
        0xA9, 0x80,         // LDA #$80
        0x85, 0x20,         // STA $20
        0xA9, 0xC0,         // LDA #$C0
        0x8D, 0x00, 0x10,   // STA $1000
        0x24, 0x1F,         // BIT $1F
        0x24, 0x20,         // BIT $20
        0x2C, 0x00, 0x10    // BIT $1000
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x00));
    T(STEP(3)); T(r8(0x001F, 0x00));
    T(STEP(2)); T(ra(0x80));
    T(STEP(3)); T(r8(0x0020, 0x80));
    T(STEP(2)); T(ra(0xC0));
    T(STEP(4)); T(r8(0x1000, 0xC0));
    T(STEP(3)); T(tf(M6502X_ZF));
    T(STEP(3)); T(tf(M6502X_NF));
    T(STEP(4)); T(tf(M6502X_NF|M6502X_VF));
}

UTEST(m6502_perfect, BNE_BEQ) {
    init();
    uint8_t prog[] = {
        0xA9, 0x10,         // LDA #$10
        0xC9, 0x10,         // CMP #$10
        0xF0, 0x02,         // BEQ eq
        0xA9, 0x0F,         // ne: LDA #$0F
        0xC9, 0x0F,         // eq: CMP #$0F
        0xD0, 0xFA,         // BNE ne -> executed 2x, second time not taken
        0xEA,
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(STEP(2)); T(ra(0x10));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(3)); T(rpc(0x0209)); // NOTE: target op has already been fetched
    T(STEP(2)); T(tf(M6502X_CF));
    T(STEP(3)); T(rpc(0x0207));
    T(STEP(2)); T(ra(0x0F));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(2)); T(rpc(0x020D));

    // patch jump target, and test jumping across 256 bytes page
    init();
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);
    w8(0x0205, 0xC0);
    T(STEP(2)); T(ra(0x10));
    T(STEP(2)); T(tf(M6502X_ZF|M6502X_CF));
    T(STEP(4)); T(rpc(0x01C7));

    // FIXME: test the other branches
}

UTEST(m6502_perfect, JMP) {
    init();
    uint8_t prog[] = {
        0x4C, 0x00, 0x10,   // JMP $1000
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);
    T(STEP(3)); T(rpc(0x1001));
}

UTEST(m6502_perfect, JMP_indirect_samepage) {
    init();
    uint8_t prog[] = {
        0xA9, 0x33,         // LDA #$33
        0x8D, 0x10, 0x21,   // STA $2110
        0xA9, 0x22,         // LDA #$22
        0x8D, 0x11, 0x21,   // STA $2111
        0x6C, 0x10, 0x21,   // JMP ($2110)
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);
    T(STEP(2)); T(ra(0x33));
    T(STEP(4)); T(r8(0x2110, 0x33));
    T(STEP(2)); T(ra(0x22));
    T(STEP(4)); T(r8(0x2111, 0x22));
    T(STEP(5)); T(rpc(0x2234));
}

UTEST(m6502_perfect, JMP_indirect_wrap) {
    init();
    uint8_t prog[] = {
        0xA9, 0x33,         // LDA #$33
        0x8D, 0xFF, 0x21,   // STA $21FF
        0xA9, 0x22,         // LDA #$22
        0x8D, 0x00, 0x21,   // STA $2100    // note: wraps around!
        0x6C, 0xFF, 0x21,   // JMP ($21FF)
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);
    T(STEP(2)); T(ra(0x33));
    T(STEP(4)); T(r8(0x21FF, 0x33));
    T(STEP(2)); T(ra(0x22));
    T(STEP(4)); T(r8(0x2100, 0x22));
    T(STEP(5)); T(rpc(0x2234));
}

UTEST(m6502_perfect, JSR_RTS) {
    init();
    uint8_t prog[] = {
        0x20, 0x05, 0x03,   // JSR fun
        0xEA, 0xEA,         // NOP, NOP
        0xEA,               // fun: NOP
        0x60,               // RTS
    };
    copy(0x0300, prog, sizeof(prog));
    prefetch(0x0300);

    T(rs(0xBD));
    T(STEP(6)); T(rpc(0x0306)); T(rs(0xBB)); T(r16(0x01BC, 0x0302));
    T(STEP(2));
    T(STEP(6)); T(rpc(0x0304)); T(rs(0xBD));
}

UTEST(m6502_perfect, RTI) {
    init();
    uint8_t prog[] = {
        0xA9, 0x11,     // LDA #$11
        0x48,           // PHA
        0xA9, 0x22,     // LDA #$22
        0x48,           // PHA
        0xA9, 0x33,     // LDA #$33
        0x48,           // PHA
        0x40,           // RTI
    };
    copy(0x0200, prog, sizeof(prog));
    prefetch(0x0200);

    T(rs(0xBD));
    T(STEP(2)); T(ra(0x11));
    T(STEP(3)); T(rs(0xBC));
    T(STEP(2)); T(ra(0x22));
    T(STEP(3)); T(rs(0xBB));
    T(STEP(2)); T(ra(0x33));
    T(STEP(3)); T(rs(0xBA));
    T(STEP(6)); T(rs(0xBD)); T(rpc(0x1123)); T(tf(M6502X_ZF|M6502X_CF));
}



