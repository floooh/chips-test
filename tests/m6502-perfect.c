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
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "utest.h"
#include <assert.h>

#define T(b) ASSERT_TRUE(b)

#define OP(c) T(step_until_sync(c,false,false))
#define OPIRQ(c) T(step_until_sync(c,true,false))
#define OPNMI(c) T(step_until_sync(c,false,true))
#define TA(v) T(ra(v))
#define TX(v) T(rx(v))
#define TY(v) T(ry(v))
#define TS(v) T(rs(v))
#define TPC(v) T(rpc(v))
#define TF(v) T(tf(v))
#define TM8(a,v) T(r8(a,v))
#define TM16(a,v) T(r16(a,v))

// perfect6502's CPU state
static void* p6502_state;

// our own emulator state and memory
static m6502_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16] = { 0 };
static uint8_t logsteps = 0;

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
    uint8_t v0 = mem[addr];
    uint8_t v1 = memory[addr];
    return (v0 == expected) && (v1 == expected);
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
    pins = m6502_init(&cpu, &(m6502_desc_t){0});
    cpu.S = 0xC0;
    if (p6502_state) {
        destroyChip(p6502_state);
        p6502_state = 0;
    }
    p6502_state = initAndResetChip();
}

// perform memory access for our own emulator
static uint64_t mem_access(uint64_t pin_mask) {
    const uint16_t addr = M6502_GET_ADDR(pin_mask);
    if (pin_mask & M6502_RW) {
        /* read */
        uint8_t val = mem[addr];
        M6502_SET_DATA(pin_mask, val);
    }
    else {
        /* write */
        uint8_t val = M6502_GET_DATA(pin_mask);
        mem[addr] = val;
    }
    return pin_mask;
}

// reset both emulators through their reset sequence to a starting address
static void start(uint16_t start_addr) {
    // write starting address to the 6502's reset vector location
    w16(0xFFFC, start_addr);

    // reset our own emulation
    for (int i = 0; i < 7; i++) {
        pins = mem_access(m6502_tick(&cpu, pins));
    }

    // run through the perfect6502 9-cycle reset sequence
    // here, the SP starts as 0xC0 and is reduced to 0xBD after the reset routine
    for (int i = 0; i < 9; i++) {
        step(p6502_state);
        step(p6502_state);
    }
    assert(readPC(p6502_state) == start_addr);
    // run one half-tick ahead into the next instruction
    step(p6502_state);
    if (logsteps) {
        printf("<-- "); chipStatus(p6502_state);
    }
    // make sure both memories have the same content
    assert(0 == memcmp(mem, memory, (1<<16)));
}

// do a single tick (two half-ticks) and check if both emulators agree
static bool step_cycle(uint32_t cur_tick) {
    bool stepok = true;
    // step our own emu
    pins = mem_access(m6502_tick(&cpu, pins));
    // step perfect6502 simulation (in half-steps)
    if (cur_tick > 0) {
        // skip the first half tick which was executed in the last invocation
        step(p6502_state);
        if (logsteps) {
            printf("<-- "); chipStatus(p6502_state);
        }
    }
    step(p6502_state);
    if (logsteps) {
        printf("<-- "); chipStatus(p6502_state);
    }
    // check whether both emulators agree on the observable pin state after each tick
    bool m6502_rw = (pins & M6502_RW);
    bool p6502_rw = readRW(p6502_state);
    if (m6502_rw != p6502_rw) {
        if (logsteps) {
            printf("RW mismatch\n");
        }
        stepok =  false;
    }
    uint16_t m6502_addr = M6502_GET_ADDR(pins);
    uint16_t p6502_addr  = readAddressBus(p6502_state);
    if (m6502_addr != p6502_addr) {
        if (logsteps) {
            printf("AD mismatch\n");
        }
        stepok =  false;
    }
    uint8_t m6502_data = M6502_GET_DATA(pins);
    uint8_t p6502_data  = readDataBus(p6502_state);
    if (m6502_data != p6502_data) {
        if (logsteps) {
            printf("D mismatch\n");
        }
        stepok =  false;
    }
    bool m6502_sync = (pins & M6502_SYNC);
    bool p6502_sync = isNodeHigh(p6502_state, 539); // 539 is SYNC pin
    if (m6502_sync != p6502_sync) {
        if (logsteps) {
            printf("SYN mismatch\n");
        }
        stepok =  false;
    }
    if (logsteps) {
        printf("<= AB:%04X DA:%02X RnW:%d SYN:%d IRQ:%d NMI:%d nmi_pip:%04x irq_pip:%04x\n",
            m6502_addr,
            m6502_data,
            m6502_rw,
            m6502_sync,
            (pins & M6502_IRQ)==0, // note: inverted output for easier comparison with perfect6502
            (pins & M6502_NMI)==0,
            cpu.nmi_pip,
            cpu.irq_pip);
    }
    return stepok;
}

// step both emulators through the current instruction,
// compare the relevant pin state after each tick, and finally
// check for expected number of ticks
static bool step_until_sync(uint32_t expected_ticks, bool irq, bool nmi) {
    uint32_t tick = 0;
    do {
        if (!step_cycle(tick++)) {
            return false;
        }
    } while (!isNodeHigh(p6502_state, 539)); // 539 is SYNC pin, next instruction about to begin
    if (irq) {
        if (logsteps) {
            printf("Assert IRQ\n");
        }
        pins |= M6502_IRQ;
        setNode(p6502_state, 103, 0);   // 103 is IRQ pin
    }
    if (nmi) {
        if (logsteps) {
            printf("Assert NMI\n");
        }
        pins |= M6502_NMI;
        setNode(p6502_state, 1297, 0);  // 1297 is NMI pin
    }
    // run one half tick into next instruction so that overlapped operations can finish
    step(p6502_state);
    if (logsteps) {
        printf("<-- "); chipStatus(p6502_state);
        printf("tick=%d exp=%d\n", tick, expected_ticks);
    }
    if (tick == expected_ticks) {
        return true;
    } else {
        return false;
    }
}

static bool single_step(uint32_t ticks, bool irq, bool nmi) {
    uint32_t tick = 0;
    do {
        if (!step_cycle(tick++)) {
            return false;
        }
    } while (tick < ticks);
    if (irq) {
        pins |= M6502_IRQ;
        setNode(p6502_state, 103, 0);   // 103 is IRQ pin
    }
    else{
        pins &= ~M6502_IRQ;
        setNode(p6502_state, 103, 1);   // 103 is IRQ pin

    }
    if (nmi) {
        pins |= M6502_NMI;
        setNode(p6502_state, 1297, 0);  // 1297 is NMI pin
    }
    else{
        pins &= ~M6502_NMI;
        setNode(p6502_state, 1297, 1);  // 1297 is NMI pin
    }
    // run one half tick into next instruction so that overlapped operations can finish
    step(p6502_state);
    if (logsteps) {
        printf("<-- "); chipStatus(p6502_state);
    }
    return true;
}
// check the flag bits and registers in both emulators against expected value
static bool tf(uint8_t expected) {
    uint8_t p6502_p = readP(p6502_state) & ~(M6502_XF|M6502_IF|M6502_BF);
    uint8_t m6502_p = cpu.P & ~(M6502_XF|M6502_IF|M6502_BF);
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
    uint16_t p6502_pc = readPC(p6502_state) - 1;
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
    start(0x0200);

    // immediate
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TA(0x01); TF(0);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TA(0x80); TF(M6502_NF);

    // zero page
    OP(3); TA(0x01); TF(0);
    OP(3); TA(0x00); TF(M6502_ZF);
    OP(3); TA(0x80); TF(M6502_NF);
    OP(3); TA(0x03); TF(0);

    // absolute
    OP(4); TA(0x12); TF(0);
    OP(4); TA(0x34); TF(0);
    OP(4); TA(0x56); TF(0);

    // zero page,X
    OP(2); TX(0x0F); TF(0);
    OP(4); TA(0xAA); TF(M6502_NF);
    OP(4); TA(0x33); TF(0);
    OP(4); TA(0x22); TF(0);

    // absolute,X
    OP(5); TA(0x12); TF(0);
    OP(4); TA(0x34); TF(0);
    OP(4); TA(0x56); TF(0);

    // absolute,Y
    OP(2); TY(0xF0); TF(M6502_NF);
    OP(5); TA(0x12); TF(0);
    OP(4); TA(0x34); TF(0);
    OP(5); TA(0x56); TF(0);

    // indirect,X
    w8(0x00FF, 0x34); w8(0x0000, 0x12); w16(0x007f, 0x4321);
    w8(0x1234, 0x89); w8(0x4321, 0x8A);
    OP(6); TA(0x89); TF(M6502_NF);
    OP(6); TA(0x8A); TF(M6502_NF);

    // indirect,Y (both 6 cycles because page boundary crossed)
    w8(0x1324, 0x98); w8(0x4411, 0xA8);
    OP(6); TA(0x98); TF(M6502_NF);
    OP(6); TA(0xA8); TF(M6502_NF);
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
    start(0x0200);

    // immediate
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0x01); TF(0);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0x80); TF(M6502_NF);

    // zero page
    OP(3); TX(0x01); TF(0);
    OP(3); TX(0x00); TF(M6502_ZF);
    OP(3); TX(0x80); TF(M6502_NF);
    OP(3); TX(0x03); TF(0);

    // absolute
    OP(4); TX(0x12); TF(0);
    OP(4); TX(0x34); TF(0);
    OP(4); TX(0x56); TF(0);

    // zero page,Y
    OP(2); TY(0x0F); TF(0);
    OP(4); TX(0xAA); TF(M6502_NF);
    OP(4); TX(0x33); TF(0);
    OP(4); TX(0x22); TF(0);

    // absolute,X
    OP(2); TY(0xF0); TF(M6502_NF);
    OP(5); TX(0x12); TF(0);
    OP(4); TX(0x34); TF(0);
    OP(5); TX(0x56); TF(0);
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
    start(0x0200);

    // immediate
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TY(0x01); TF(0);
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TY(0x80); TF(M6502_NF);

    // zero page
    OP(3); TY(0x01); TF(0);
    OP(3); TY(0x00); TF(M6502_ZF);
    OP(3); TY(0x80); TF(M6502_NF);
    OP(3); TY(0x03); TF(0);

    // absolute
    OP(4); TY(0x12); TF(0);
    OP(4); TY(0x34); TF(0);
    OP(4); TY(0x56); TF(0);

    // zero page,Y
    OP(2); TX(0x0F); TF(0);
    OP(4); TY(0xAA); TF(M6502_NF);
    OP(4); TY(0x33); TF(0);
    OP(4); TY(0x22); TF(0);

    // absolute,X
    OP(2); TX(0xF0); TF(M6502_NF);
    OP(5); TY(0x12); TF(0);
    OP(4); TY(0x34); TF(0);
    OP(5); TY(0x56); TF(0);
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
    start(0x0200);

    OP(2); TA(0x23);
    OP(2); TX(0x10);
    OP(2); TY(0xC0);
    OP(3); TM8(0x0010, 0x23);
    OP(4); TM8(0x1234, 0x23);
    OP(4); TM8(0x0020, 0x23);
    OP(5); TM8(0x2010, 0x23);
    OP(5); TM8(0x20C0, 0x23);
    w16(0x0020, 0x4321);
    OP(6); TM8(0x4321, 0x23);
    OP(6); TM8(0x43E1, 0x23);
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
    start(0x0200);

    OP(2); TX(0x23);
    OP(2); TY(0x10);
    OP(3); TM8(0x0010, 0x23);
    OP(4); TM8(0x1234, 0x23);
    OP(4); TM8(0x0020, 0x23);
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
    start(0x0200);

    OP(2); TY(0x23);
    OP(2); TX(0x10);
    OP(3); TM8(0x0010, 0x23);
    OP(4); TM8(0x1234, 0x23);
    OP(4); TM8(0x0020, 0x23);
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
    start(0x0200);

    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TX(0x10); TF(0);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TA(0xF0); TF(M6502_NF);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TA(0xF0); TF(M6502_NF);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0xF0); TF(M6502_NF);
    OP(2); TA(0x01); TF(0);
    OP(2); TA(0xF0); TF(M6502_NF);
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
    start(0x0200);

    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TY(0x10); TF(0);
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TA(0xF0); TF(M6502_NF);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TA(0xF0); TF(M6502_NF);
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TY(0xF0); TF(M6502_NF);
    OP(2); TA(0x01); TF(0);
    OP(2); TA(0xF0); TF(M6502_NF);
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
    start(0x0200);

    OP(2); TX(0x01); TF(0);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0xFF); TF(M6502_NF);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0x01); TF(0);
    OP(2); TY(0x01); TF(0);
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TY(0xFF); TF(M6502_NF);
    OP(2); TY(0x00); TF(M6502_ZF);
    OP(2); TY(0x01); TF(0);
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
    start(0x0200);

    OP(2); TX(0xAA); TF(M6502_NF);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TS(0xAA); TF(M6502_ZF);
    OP(2); TX(0x00); TF(M6502_ZF);
    OP(2); TX(0xAA); TF(M6502_NF);
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
    start(0x0200);

    OP(2); TA(0x00); TF(M6502_ZF);
    OP(2); TX(0x01); TF(0);
    OP(2); TY(0x02); TF(0);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(3); TA(0x01); TF(0);
    OP(4); TA(0x03); TF(0);
    OP(4); TA(0x07); TF(0);
    OP(4); TA(0x0F); TF(0);
    OP(4); TA(0x1F); TF(0);
    OP(6); TA(0x3F); TF(0);
    OP(5); TA(0x7F); TF(0);
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
    start(0x0200);

    OP(2); TA(0xFF); TF(M6502_NF);
    OP(2); TX(0x01); TF(0);
    OP(2); TY(0x02); TF(0);
    OP(2); TA(0xFF); TF(M6502_NF);
    OP(3); TA(0x7F); TF(0);
    OP(4); TA(0x3F); TF(0);
    OP(4); TA(0x1F); TF(0);
    OP(4); TA(0x0F); TF(0);
    OP(4); TA(0x07); TF(0);
    OP(6); TA(0x03); TF(0);
    OP(5); TA(0x01); TF(0);
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
    start(0x0200);

    OP(2); TA(0xFF); TF(M6502_NF);
    OP(2); TX(0x01); TF(0);
    OP(2); TY(0x02); TF(0);
    OP(2); TA(0x00); TF(M6502_ZF);
    OP(3); TA(0x7F); TF(0);
    OP(4); TA(0x40); TF(0);
    OP(4); TA(0x5F); TF(0);
    OP(4); TA(0x50); TF(0);
    OP(4); TA(0x57); TF(0);
    OP(6); TA(0x54); TF(0);
    OP(5); TA(0x55); TF(0);
}

UTEST(m6502_perfect, NOP) {
    init();
    uint8_t prog[] = {
        0xEA,       // NOP
    };
    copy(0x0200, prog, sizeof(prog));
    start(0x0200);
    OP(2);
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
    start(0x0200);

    OP(2); TA(0x23); TS(0xBD);
    OP(3); TS(0xBC); TM8(0x01BD, 0x23);
    OP(2); TA(0x32);
    OP(4); TA(0x23); TS(0xBD); TF(0);
    OP(3); TS(0xBC); TM8(0x01BD, M6502_XF|M6502_IF|M6502_BF);
    OP(2); TF(M6502_ZF);
    OP(4); TS(0xBD); TF(0);
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
    start(0x0200);

    OP(2); TF(M6502_ZF);
    OP(2); //TF(M6502_ZF|M6502_IF);   // FIXME: interrupt bit is ignored in tf()
    OP(2); TF(M6502_ZF);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(2); TF(M6502_ZF);
    OP(2); TF(M6502_ZF|M6502_DF);
    OP(2); TF(M6502_ZF);
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
    start(0x0200);

    OP(2); TX(0x10);
    OP(5); TM8(0x0033,1); TF(0);
    OP(6); TM8(0x0043,1); TF(0);
    OP(6); TM8(0x1000,1); TF(0);
    OP(7); TM8(0x1010,1); TF(0);
    OP(5); TM8(0x0033,0); TF(M6502_ZF);
    OP(6); TM8(0x0043,0); TF(M6502_ZF);
    OP(6); TM8(0x1000,0); TF(M6502_ZF);
    OP(7); TM8(0x1010,0); TF(M6502_ZF);
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
    start(0x0200);

    OP(2); TA(0x01);
    OP(3); TM8(0x0010, 0x01);
    OP(4); TM8(0x1000, 0x01);
    OP(2); TA(0xFC);
    OP(2); TX(0x08);
    OP(2); TY(0x04)
    OP(2);  // CLC
    // ADC
    OP(2); TA(0xFD); TF(M6502_NF);
    OP(3); TA(0xFE); TF(M6502_NF);
    OP(4); TA(0xFF); TF(M6502_NF);
    OP(4); TA(0x00); TF(M6502_ZF|M6502_CF);
    OP(5); TA(0x02); TF(0);
    OP(5); TA(0x03); TF(0);
    // SBC
    OP(5); TA(0x01); TF(M6502_CF);
    OP(5); TA(0x00); TF(M6502_ZF|M6502_CF);
    OP(4); TA(0xFF); TF(M6502_NF);
    OP(4); TA(0xFD); TF(M6502_NF|M6502_CF);
    OP(3); TA(0xFC); TF(M6502_NF|M6502_CF);
    OP(2); TA(0xFB); TF(M6502_NF|M6502_CF);
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
    start(0x0200);

    OP(2); TA(0x01);
    OP(2); TX(0x02);
    OP(2); TY(0x03);
    OP(2); TF(M6502_CF);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(2); TF(M6502_NF);
    OP(2); TF(M6502_CF);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(2); TF(M6502_NF);
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
    start(0x0200);

    OP(2); TA(0x81);
    OP(2); TX(0x01);
    OP(3); TM8(0x0010, 0x81);
    OP(5); TM8(0x0010, 0x02); TF(M6502_CF);
    OP(6); TM8(0x0010, 0x04); TF(0);
    OP(2); TA(0x02); TF(M6502_CF);
    OP(2); TA(0x04); TF(0);
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
    start(0x0200);

    OP(2); TA(0x81);
    OP(2); TX(0x01);
    OP(3); TM8(0x0010, 0x81);
    OP(5); TM8(0x0010, 0x40); TF(M6502_CF);
    OP(6); TM8(0x0010, 0x20); TF(0);
    OP(2); TA(0x40); TF(M6502_CF);
    OP(2); TA(0x20); TF(0);
}

UTEST(m6502_perfect, ROR_ROL) {
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
    start(0x0200);

    OP(2); TA(0x81);
    OP(2); TX(0x01);
    OP(3); TM8(0x0010, 0x81);
    OP(5); TM8(0x0010, 0x02); TF(M6502_CF);
    OP(6); TM8(0x0010, 0x05); TF(0);
    OP(6); TM8(0x0010, 0x02); TF(M6502_CF);
    OP(5); TM8(0x0010, 0x81); TF(M6502_NF);
    OP(2); TA(0x40); TF(M6502_CF);
    OP(2); TA(0xA0); TF(M6502_NF);
    OP(2); TA(0x40); TF(M6502_CF);
    OP(2); TA(0x81); TF(M6502_NF);
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
    start(0x0200);

    OP(2); TA(0x00);
    OP(3); TM8(0x001F, 0x00);
    OP(2); TA(0x80);
    OP(3); TM8(0x0020, 0x80);
    OP(2); TA(0xC0);
    OP(4); TM8(0x1000, 0xC0);
    OP(3); TF(M6502_ZF);
    OP(3); TF(M6502_NF);
    OP(4); TF(M6502_NF|M6502_VF);
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
    start(0x0200);

    OP(2); TA(0x10);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(3); TPC(0x0208);
    OP(2); TF(M6502_CF);
    OP(3); T(rpc(0x0206));
    OP(2); TA(0x0F);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(2); TPC(0x020C);

    // patch jump target, and test jumping across 256 bytes page
    init();
    copy(0x0200, prog, sizeof(prog));
    start(0x0200);
    w8(0x0205, 0xC0);
    OP(2); TA(0x10);
    OP(2); TF(M6502_ZF|M6502_CF);
    OP(4); TPC(0x01C6);

    // FIXME: test the other branches
}

UTEST(m6502_perfect, JMP) {
    init();
    uint8_t prog[] = {
        0x4C, 0x00, 0x10,   // JMP $1000
    };
    copy(0x0200, prog, sizeof(prog));
    start(0x0200);
    OP(3); TPC(0x1000);
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
    start(0x0200);
    OP(2); TA(0x33);
    OP(4); TM8(0x2110, 0x33);
    OP(2); TA(0x22);
    OP(4); TM8(0x2111, 0x22);
    OP(5); TPC(0x2233);
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
    start(0x0200);
    OP(2); TA(0x33);
    OP(4); TM8(0x21FF, 0x33);
    OP(2); TA(0x22);
    OP(4); TM8(0x2100, 0x22);
    OP(5); TPC(0x2233);
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
    start(0x0300);

    TS(0xBD);
    OP(6); TPC(0x0305); TS(0xBB); TM16(0x01BC, 0x0302);
    OP(2);
    OP(6); TPC(0x0303); TS(0xBD);
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
    start(0x0200);

    TS(0xBD);
    OP(2); TA(0x11);
    OP(3); TS(0xBC);
    OP(2); TA(0x22);
    OP(3); TS(0xBB);
    OP(2); TA(0x33);
    OP(3); TS(0xBA);
    OP(6); TS(0xBD); TPC(0x1122); TF(M6502_ZF|M6502_CF);
}

UTEST(m6502_perfect, BRK) {
    init();
    uint8_t prog[] = {
        0xA9, 0xAA,     // LDA #$AA
        0x00,           // BRK
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xA9, 0xBB,     // LDA #$BB
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    w16(0xFFFE, 0x0010);

    OP(2); TA(0xAA);
    OP(7); TS(0xBA); TPC(0x0010); TS(0xBA); TM8(0x1BB, 0xB4); TM8(0x1BC,0x04); TM8(0x1BD,0x00);
    OP(2); TA(0xBB);
}

UTEST(m6502_perfect, IRQ) {
    init();
    uint8_t prog[] = {
        0x58, 0xEA, 0xEA, 0xEA, 0xEA,   // CLI + 4 nops
        0xA9, 0x33,                     // IRQ service routine
        0xA9, 0x22,
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    w16(0xFFFE, 0x5);

    OP(2);          // enable interrupt
    OPIRQ(2);       // run NOP, and set IRQ before next op starts
    OP(2);          // next NOP runs as usual...
    OP(7);          // IRQ running
    OP(2); TA(0x33) // first LDA
    OP(2); TA(0x22) // second LDA
}

UTEST(m6502_perfect, NMI) {
    init();
    uint8_t prog[] = {
        0xEA, 0xEA, 0xEA, 0xEA, 0xEA,   // no CLI
        0xA9, 0x33,                     // interrupt service routine
        0xA9, 0x22,
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    w16(0xFFFA, 0x5);

    OP(2);          // enable interrupt
    OPNMI(2);       // run NOP, and set NMI before next op starts
    OP(2);          // next NOP runs as usual...
    OP(7);          // NMI running
    OP(2); TA(0x33) // first LDA
    OP(2); TA(0x22) // second LDA
}

#define SINGLE_PULSE_INT 0
UTEST(m6502_perfect, NMI2) {
//        logsteps = 1;
    for (int nmi_delay = 0; nmi_delay < 9; nmi_delay++)
    //int nmi_delay = 7;
    {
        if (logsteps) {
            printf("---------------------- NMI delay %d\n", nmi_delay);
        }
        init();
        uint8_t prog[] = {
            0xEA,         // NOP
            0xA9, 0x00,   // LDA #0
            0xD0, 0xFC,   // BEQ $0000
            0xF0, 0xFA,   // BEQ $0000
            0xEA,         // NOP
            0xA9, 0x33,   // interrupt service routine
            0xA9, 0x22,
        };
        copy(0x0, prog, sizeof(prog));
        start(0x0);
        w16(0xFFFA, 0x8);

        T(single_step(1, false, false));
        T(single_step(nmi_delay, false, true));
#if SINGLE_PULSE_INT
        T(single_step(0, false, false));
        T(single_step(10, false, false));
#else
        T(single_step(10, false, true));
#endif
    }
}

UTEST(m6502_perfect, IRQ2) {
//        logsteps = 1;
    for (int irq_delay = 0; irq_delay < 9; irq_delay++) {
        if (logsteps) {
            printf("---------------------- IRQ delay %d\n", irq_delay);
        }
        init();
        uint8_t prog[] = {
            0x58,         // CLI
            0xA9, 0x00,   // LDA #0
            0xD0, 0xFC,   // BEQ $0000
            0xF0, 0xFA,   // BEQ $0000
            0xEA,         // NOP
            0xA9, 0x33,   // interrupt service routine
            0xA9, 0x22,
        };
        copy(0x0, prog, sizeof(prog));
        start(0x0);
        w16(0xFFFA, 0x8);

        T(single_step(1, false, false));
        T(single_step(irq_delay, true, false));
#if SINGLE_PULSE_INT
        T(single_step(0, false, false));
        T(single_step(10, false, false));
#else
        T(single_step(10, true, false));
#endif
    }
}

UTEST(m6502_perfect, ADC_decimal_0) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=30&a=0&d=a9c848a900286900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xa9, 0xc8, // LDA #C8
        0x48,       // PHA
        0xa9, 0x00, // LDA #0
        0x28,       // PLP
        0x69, 0x00, // ADC #00
        0xea,       // NOP
        0x08,       // PHP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xC8);                // LDA %C8
    OP(3); TF(M6502_NF);            // PLP
    OP(2); TA(0x00); TF(M6502_ZF);  // LDA #0
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF); // PLP
    OP(2); TA(0x00); TF(M6502_DF|M6502_ZF);   // ADC #00
    OP(2);                          // NOP
}

UTEST(m6502_perfect, ADC_decimal_1) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a90f48a979286900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x0F, // LDA #0F
        0x48,       // PHA
        0xA9, 0x79, // LDA #79
        0x28,       // PLP
        0x69, 0x00, // ADC #0
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP,
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x0F);        // LDA #0F
    OP(3); TF(0x00);        // PHA
    OP(2); TA(0x79);        // LDA #79
    OP(4); TF(M6502_DF|M6502_ZF|M6502_CF);  // PLP
    OP(2); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);  // ADC #0
    OP(2); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);  // NOP
    OP(3); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);  // PHP
    OP(2); TA(0x80); TX(0x80); TF(M6502_NF|M6502_VF|M6502_DF);  // TAX
    OP(4); TA(0xFC); TX(0x80); TF(M6502_NF|M6502_VF|M6502_DF);  // PLA
    OP(2); TA(0x3E); TX(0x80); TF(M6502_VF|M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_2) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a90a48a924286956ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x0A, // LDA #0A
        0x48,       // PHA
        0xA9, 0x24, // LDA #24
        0x28,       // PLP
        0x69, 0x56, // ADC #56
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x0A);        // LDA #0A
    OP(3); TF(0x00);        // PHA
    OP(2); TA(0x24);        // LDA #24
    OP(4); TF(M6502_DF|M6502_ZF);    // PLP
    OP(2); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);    // ADC #56
    OP(2); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);    // NOP
    OP(3); TA(0x80); TF(M6502_NF|M6502_VF|M6502_DF);    // PHP
    OP(2); TA(0x80); TX(0x80); TF(M6502_NF|M6502_VF|M6502_DF);    // TAX
    OP(4); TA(0xF8); TX(0x80); TF(M6502_NF|M6502_VF|M6502_DF);    // PLA
    OP(2); TA(0x3A); TX(0x80); TF(M6502_VF|M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_3) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a98e48a993286982ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x8E, // LDA #8E
        0x48,       // PHA
        0xA9, 0x93, // LDA #93
        0x28,       // PLP
        0x69, 0x82, // ADC #82
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x8E);        // LDA #8E
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x93);        // LDA #93
    OP(4); TF(M6502_NF|M6502_DF|M6502_ZF);    // PLP
    OP(2); TA(0x75); TF(M6502_VF|M6502_DF|M6502_CF);    // ADC #82
    OP(2); TA(0x75); TF(M6502_VF|M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0x75); TF(M6502_VF|M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0x75); TX(0x75); TF(M6502_VF|M6502_DF|M6502_CF);    // TAX
    OP(4); TA(0x7D); TX(0x75); TF(M6502_VF|M6502_DF|M6502_CF);    // PLA
    OP(2); TA(0xBF); TX(0x75); TF(M6502_NF|M6502_VF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_4) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9fe48a989286976ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFE, // LDA #FE
        0x48,       // PHA
        0xA9, 0x89, // LDA #89
        0x28,       // PLP
        0x69, 0x76, // ADC #76
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFE);        // LDA #FE
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x89);        // LDA #89
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF);     // PLP
    OP(2); TA(0x65); TF(M6502_DF|M6502_CF);    // ADC #76
    OP(2); TA(0x65); TF(M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0x65); TF(M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0x65); TX(0x65); TF(M6502_DF|M6502_CF);    // TAX
    OP(4); TA(0x3D); TX(0x65); TF(M6502_DF|M6502_CF);    // PLA
    OP(2); TA(0xFF); TX(0x65); TF(M6502_NF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_5) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9fd48a989286976ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFD, // LDA #FD
        0x48,       // PHA
        0xA9, 0x89, // LDA #89
        0x28,       // PLP
        0x69, 0x76, // ADC #76
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFD);        // LDA #FD
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x89);        // LDA #89
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_CF);     // PLP
    OP(2); TA(0x66); TF(M6502_DF|M6502_ZF|M6502_CF);    // ADC #76
    OP(2); TA(0x66); TF(M6502_DF|M6502_ZF|M6502_CF);    // NOP
    OP(3); TA(0x66); TF(M6502_DF|M6502_ZF|M6502_CF);    // PHP
    OP(2); TA(0x66); TX(0x66); TF(M6502_DF|M6502_CF);   // TAX
    OP(4); TA(0x3F); TX(0x66); TF(M6502_DF|M6502_CF);   // PLA
    OP(2); TA(0xFD); TX(0x66); TF(M6502_NF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_6) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9ba48a9802869f0ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xBA, // LDA #BA
        0x48,       // PHA
        0xA9, 0x80, // LDA #80
        0x28,       // PLP
        0x69, 0xF0, // ADC #F0
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xBA);        // LDA #BA
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x80);        // LDA #80
    OP(4); TF(M6502_NF|M6502_DF|M6502_ZF);     // PLP
    OP(2); TA(0xD0); TF(M6502_VF|M6502_DF|M6502_CF);    // ADC #F0
    OP(2); TA(0xD0); TF(M6502_VF|M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0xD0); TF(M6502_VF|M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0xD0); TX(0xD0); TF(M6502_NF|M6502_VF|M6502_DF|M6502_CF);   // TAX
    OP(4); TA(0x79); TX(0xD0); TF(M6502_VF|M6502_DF|M6502_CF);   // PLA
    OP(2); TA(0xBB); TX(0xD0); TF(M6502_NF|M6502_VF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_7) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a97e48a9802869faea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x7E, // LDA #7E
        0x48,       // PHA
        0xA9, 0x80, // LDA #80
        0x28,       // PLP
        0x69, 0xFA, // ADC #FA
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x7E);        // LDA #7E
    OP(3); TF(0);           // PHA
    OP(2); TA(0x80); TF(M6502_NF);  // LDA #80
    OP(4); TF(M6502_VF|M6502_DF|M6502_ZF);     // PLP
    OP(2); TA(0xE0); TF(M6502_NF|M6502_DF|M6502_CF);    // ADC #FA
    OP(2); TA(0xE0); TF(M6502_NF|M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0xE0); TF(M6502_NF|M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0xE0); TX(0xE0); TF(M6502_NF|M6502_DF|M6502_CF);   // TAX
    OP(4); TA(0xBD); TX(0xE0); TF(M6502_NF|M6502_DF|M6502_CF);   // PLA
    OP(2); TA(0x7F); TX(0xE0); TF(M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_8) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9fe48a92f28694fea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFE, // LDA #FE
        0x48,       // PHA
        0xA9, 0x2F, // LDA #2D
        0x28,       // PLP
        0x69, 0x4F, // ADC #4F
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFE); TF(M6502_NF); // LDA #FE
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x2F); TF(0);  // LDA #2F
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x74); TF(M6502_DF);    // ADC #4F
    OP(2); TA(0x74); TF(M6502_DF);    // NOP
    OP(3); TA(0x74); TF(M6502_DF);    // PHP
    OP(2); TA(0x74); TX(0x74); TF(M6502_DF);   // TAX
    OP(4); TA(0x3C); TX(0x74); TF(M6502_DF);   // PLA
    OP(2); TA(0xFE); TX(0x74); TF(M6502_NF|M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_9) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9ff48a96f286900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFF, // LDA 0xFF
        0x48,       // PHA
        0xA9, 0x6F, // LDA #6F
        0x28,       // PLP
        0x69, 0x00, // ADC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFF); TF(M6502_NF); // LDA #FF
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x6F); TF(0);  // LDA #6F
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x76); TF(M6502_DF);    // ADC #00
    OP(2); TA(0x76); TF(M6502_DF);    // NOP
    OP(3); TA(0x76); TF(M6502_DF);    // PHP
    OP(2); TA(0x76); TX(0x76); TF(M6502_DF);   // TAX
    OP(4); TA(0x3C); TX(0x76); TF(M6502_DF);   // PLA
    OP(2); TA(0xFE); TX(0x76); TF(M6502_NF|M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_99_00_c) {
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9ff48a999286900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFF, // LDA 0xFF
        0x48,       // PHA
        0xA9, 0x99, // LDA #99
        0x28,       // PLP
        0x69, 0x00, // ADC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFF); TF(M6502_NF); // LDA #FF
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x99); TF(M6502_NF);  // LDA #99
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // ADC #00
    OP(2); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0x00); TX(0x00); TF(M6502_DF|M6502_ZF|M6502_CF);   // TAX
    OP(4); TA(0xBD); TX(0x00); TF(M6502_NF|M6502_DF|M6502_CF);   // PLA
    OP(2); TA(0x7F); TX(0x00); TF(M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_00_99_c) {
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9ff48a900286999ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFF, // LDA 0xFF
        0x48,       // PHA
        0xA9, 0x00, // LDA #00
        0x28,       // PLP
        0x69, 0x99, // ADC #99
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFF); TF(M6502_NF); // LDA #FF
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x00); TF(M6502_ZF);  // LDA #00
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // ADC #99
    OP(2); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // NOP
    OP(3); TA(0x00); TF(M6502_NF|M6502_DF|M6502_CF);    // PHP
    OP(2); TA(0x00); TX(0x00); TF(M6502_DF|M6502_ZF|M6502_CF);   // TAX
    OP(4); TA(0xBD); TX(0x00); TF(M6502_NF|M6502_DF|M6502_CF);   // PLA
    OP(2); TA(0x7F); TX(0x00); TF(M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_99_00) {
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9fe48a999286900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFE, // LDA 0xFE
        0x48,       // PHA
        0xA9, 0x99, // LDA #99
        0x28,       // PLP
        0x69, 0x00, // ADC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFE); TF(M6502_NF); // LDA #FE
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x99); TF(M6502_NF);  // LDA #99
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF);    // ADC #00
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF);    // NOP
    OP(3); TA(0x99); TF(M6502_NF|M6502_DF);    // PHP
    OP(2); TA(0x99); TX(0x99); TF(M6502_NF|M6502_DF); // TAX
    OP(4); TA(0xBC); TX(0x99); TF(M6502_NF|M6502_DF); // PLA
    OP(2); TA(0x7E); TX(0x99); TF(M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, ADC_decimal_00_99) {
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9fe48a900286999ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xFE, // LDA 0xFE
        0x48,       // PHA
        0xA9, 0x00, // LDA #00
        0x28,       // PLP
        0x69, 0x99, // ADC #99
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xFE); TF(M6502_NF); // LDA #FE
    OP(3); TF(M6502_NF);    // PHA
    OP(2); TA(0x00); TF(M6502_ZF);  // LDA #00
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF);    // ADC #99
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF);    // NOP
    OP(3); TA(0x99); TF(M6502_NF|M6502_DF);    // PHP
    OP(2); TA(0x99); TX(0x99); TF(M6502_NF|M6502_DF); // TAX
    OP(4); TA(0xBC); TX(0x99); TF(M6502_NF|M6502_DF); // PLA
    OP(2); TA(0x7E); TX(0x99); TF(M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_0) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a94e48a90028e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x4E, // LDA #4E
        0x48,       // PHA
        0xA9, 0x00, // LDA #00,
        0x28,       // PLP
        0xE9, 0x00, // SBC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x4E); TF(0); // LDA #4D
    OP(3); TF(0);           // PHA
    OP(2); TA(0x00); TF(M6502_ZF); // LDA #00
    OP(4); TF(M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF); // SBC #00
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF); // NOP
    OP(3); TA(0x99); TF(M6502_NF|M6502_DF); // PHP
    OP(2); TA(0x99); TX(0x99); TF(M6502_NF|M6502_DF); // TAX
    OP(4); TA(0xBC); TX(0x99); TF(M6502_NF|M6502_DF); // PLA
    OP(2); TA(0x7E); TX(0x99); TF(M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_1) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9c948a90028e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xC9, // LDA #C9
        0x48,       // PHA
        0xA9, 0x00, // LDA #00
        0x28,       // PLP
        0xE9, 0x00, // SBC #0
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xC9); TF(M6502_NF); // LDA #C9
    OP(3); TF(M6502_NF); // PHA
    OP(2); TA(0x00); TF(M6502_ZF); // LDA #00
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_CF); // PLP
    OP(2); TA(0x00); TF(M6502_DF|M6502_ZF|M6502_CF); // SBC #00
    OP(2); TA(0x00); TF(M6502_DF|M6502_ZF|M6502_CF); // NOP
    OP(3); TA(0x00); TF(M6502_DF|M6502_ZF|M6502_CF); // PHP
    OP(2); TA(0x00); TX(0x00); TF(M6502_DF|M6502_ZF|M6502_CF); // TAX
    OP(4); TA(0x3B); TX(0x00); TF(M6502_DF|M6502_CF); // PLA
    OP(2); TA(0xF9); TX(0x00); TF(M6502_NF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_2) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a97f48a90028e901ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x7F, // LDA #7F
        0x48,       // PHA
        0xA9, 0x00, // LDA #00
        0x28,       // PLP
        0xE9, 0x01, // SBC #01
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x7F); TF(0); // LDA #7F
    OP(3); TF(0); // PHA
    OP(2); TA(0x00); TF(M6502_ZF); // LDA #00
    OP(4); TF(M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF); // SBC #01
    OP(2); TA(0x99); TF(M6502_NF|M6502_DF); // NOP
    OP(3); TA(0x99); TF(M6502_NF|M6502_DF); // PHP
    OP(2); TA(0x99); TX(0x99); TF(M6502_NF|M6502_DF); // TAX
    OP(4); TA(0xBC); TX(0x99); TF(M6502_NF|M6502_DF); // PLA
    OP(2); TA(0x7E); TX(0x99); TF(M6502_DF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_3) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9cb48a90a28e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xCB, // LDA #CB
        0x48,       // PHA
        0xA9, 0x0A, // LDA #0A
        0x28,       // PLP
        0xE9, 0x00, // SBC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xCB); TF(M6502_NF); // LDA #CB
    OP(3); TF(M6502_NF); // PHA
    OP(2); TA(0x0A); TF(0); // LDA #0A
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x0A); TF(M6502_DF|M6502_CF); // SBC #00
    OP(2); TA(0x0A); TF(M6502_DF|M6502_CF); // NOP
    OP(3); TA(0x0A); TF(M6502_DF|M6502_CF); // PHP
    OP(2); TA(0x0A); TX(0x0A); TF(M6502_DF|M6502_CF); // TAX
    OP(4); TA(0x39); TX(0x0A); TF(M6502_DF|M6502_CF); // PLA
    OP(2); TA(0xFB); TX(0x0A); TF(M6502_NF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_4) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a9ca48a90b28e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0xCA, // LDA #CA
        0x48,       // PHA
        0xA9, 0x0B, // LDA #0B
        0x28,       // PLP
        0xE9, 0x00, // SBC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0xCA); TF(M6502_NF); // LDA #CA
    OP(3); TF(M6502_NF); // PHA
    OP(2); TA(0x0B); TF(0); // LDA #0B
    OP(4); TF(M6502_NF|M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x0A); TF(M6502_DF|M6502_CF); // SBC #00
    OP(2); TA(0x0A); TF(M6502_DF|M6502_CF); // NOP
    OP(3); TA(0x0A); TF(M6502_DF|M6502_CF); // PHP
    OP(2); TA(0x0A); TX(0x0A); TF(M6502_DF|M6502_CF); // TAX
    OP(4); TA(0x39); TX(0x0A); TF(M6502_DF|M6502_CF); // PLA
    OP(2); TA(0xFB); TX(0x0A); TF(M6502_NF|M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_5) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a94b48a99a28e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x4B, // LDA #4B
        0x48,       // PHA
        0xA9, 0x9A, // LDA #9A
        0x28,       // PLP
        0xE9, 0x00, // SBC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x4B); TF(0); // LDA #4B
    OP(3); TF(0); // PHA
    OP(2); TA(0x9A); TF(M6502_NF); // LDA #9A
    OP(4); TF(M6502_VF|M6502_DF|M6502_ZF|M6502_CF); // PLP
    OP(2); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // SBC #00
    OP(2); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // NOP
    OP(3); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // PHP
    OP(2); TA(0x9A); TX(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // TAX
    OP(4); TA(0xB9); TX(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // PLA
    OP(2); TA(0x7B); TX(0x9A); TF(M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST(m6502_perfect, SBC_decimal_6) {
    // https://www.nesdev.org/wiki/Visual6502wiki/6502DecimalMode
    // http://visual6502.org/JSSim/expert.html?graphics=f&steps=56&a=0&d=a94a48a99b28e900ea08aa6849c2ea
    init();
    uint8_t prog[] = {
        0xA9, 0x4A, // LDA #4A
        0x48,       // PHA
        0xA9, 0x9B, // LDA #9B
        0x28,       // PLP
        0xE9, 0x00, // SBC #00
        0xEA,       // NOP
        0x08,       // PHP
        0xAA,       // TAX
        0x68,       // PLA
        0x49, 0xC2, // EOR #C2
        0xEA,       // NOP
    };
    copy(0x0, prog, sizeof(prog));
    start(0x0);
    OP(2); TA(0x4A); TF(0); // LDA #4A
    OP(3); TF(0); // PHA
    OP(2); TA(0x9B); TF(M6502_NF); // LDA #9B
    OP(4); TF(M6502_VF|M6502_DF|M6502_ZF); // PLP
    OP(2); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // SBC #00
    OP(2); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // NOP
    OP(3); TA(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // PHP
    OP(2); TA(0x9A); TX(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // TAX
    OP(4); TA(0xB9); TX(0x9A); TF(M6502_NF|M6502_DF|M6502_CF); // PLA
    OP(2); TA(0x7B); TX(0x9A); TF(M6502_DF|M6502_CF); // EOR #C2
    OP(2); // NOP
}

UTEST_MAIN();
