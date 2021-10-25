//------------------------------------------------------------------------------
//  z80x-int.c
//
//  Test Z80 interrupt handling and timing.
//------------------------------------------------------------------------------
#include "utest.h"
#define CHIPS_IMPL
#include "chips/z80x.h"

#define T(b) ASSERT_TRUE(b)

static z80_t cpu;
static uint64_t pins;
static uint8_t mem[(1<<16)];

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
}

static bool pins_none(void) {
    return (pins & Z80_CTRL_PIN_MASK) == 0;
}

static bool pins_m1(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_M1|Z80_MREQ|Z80_RD);
}

static bool pins_rfsh(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RFSH);
}

static bool pins_mread(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_RD);
}

static bool pins_mwrite(void) {
    return (pins & Z80_CTRL_PIN_MASK) == (Z80_MREQ|Z80_WR);
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = z80_init(&cpu);
    copy(start_addr, bytes, num_bytes);
    // run through the initial NOP
    tick();
    tick();
    tick();
}

UTEST(z80, NMI) {
    uint8_t prog[] = {
        0xFB,               //       EI
        0x21, 0x11, 0x11,   // loop: LD HL, 1111h
        0x11, 0x22, 0x22,   //       LD DE, 2222h
        0xC3, 0x01, 0x00,   //       JP loop 
    };
    uint8_t isr[] = {
        0x3E, 0x33,         //       LD A,33h
        0xED, 0x45,         //       RETN    
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0066, isr, sizeof(isr));
    cpu.sp = 0x0100;

    // EI
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none()); T(!cpu.iff1); T(!cpu.iff2);

    // LD HL,1111h
    tick(); T(pins_m1()); T(cpu.iff1); T(cpu.iff2);
    tick(); T(pins_none());
    tick(); T(pins_rfsh()); T(cpu.int_bits == 0);
    tick(); T(pins_none());
    tick(); T(pins_mread()); pins |= Z80_NMI;
    tick(); T(pins_none()); pins &= ~Z80_NMI;
    tick(); T(pins_none()); T(cpu.int_bits == Z80_NMI);
    tick(); T(pins_mread());
    tick(); T(pins_none());
    tick(); T(pins_none());

    // the NMI should kick in here, starting with a regular refresh cycle
    tick(); T(pins_m1()); T(cpu.pc == 4);
    tick(); T(pins_none()); T(cpu.pc == 4); T(!cpu.iff1);
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // extra tick
    tick(); T(pins_none());
    // mwrite
    tick(); T(pins_none()); T(cpu.pc == 4);
    tick(); T(pins_mwrite()); T(cpu.sp == 0x00FF); T(mem[0x00FF] == 0);
    tick(); T(pins_none());
    // mwrite
    tick(); T(pins_none());
    tick(); T(pins_mwrite()); T(cpu.sp == 0x00FE); T(mem[0x00FE] == 4);
    tick(); T(pins_none());

    // first overlapped tick of interrupt service routine
    tick(); T(pins_m1()); T(cpu.pc == 0x67);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_mread());
    tick(); T(pins_none());
    tick(); T(pins_none());

    // RETN
    // ED prefix
    tick(); T(pins_m1()); T(cpu.a == 0x33);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // opcode
    tick(); T(pins_m1());
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    // mread
    tick(); T(pins_mread()); T(cpu.sp == 0x00FF);
    tick(); T(pins_none()); T(cpu.pch == 0x00);
    tick(); T(pins_none());
    // mread
    tick(); T(pins_mread()); T(cpu.sp == 0x0100);
    tick(); T(pins_none()); T(cpu.pcl == 0x04);
    tick(); T(pins_none()); T(!cpu.iff1);

    // continue at LD DE,2222h
    tick(); T(pins_m1()); T(cpu.iff1); T(pins & Z80_RETI);
    tick(); T(pins_none());
    tick(); T(pins_rfsh());
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(cpu.e == 0x22);
    tick(); T(pins_none());
    tick(); T(pins_mread());
    tick(); T(pins_none()); T(cpu.d == 0x22);
    tick(); T(pins_none());
}

UTEST_MAIN()
