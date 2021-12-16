//------------------------------------------------------------------------------ 
//  m6502-int.c
//
//  Test 6502 interrupt timing.
//------------------------------------------------------------------------------ 
#include "utest.h"
#define CHIPS_IMPL
#include "chips/m6502.h"

#define T(b) ASSERT_TRUE(b)

static m6502_t cpu;
static uint64_t pins;
static uint8_t mem[1<<16];

static void tick(void) {
    pins = m6502_tick(&cpu, pins);
    const uint16_t addr = M6502_GET_ADDR(pins);
    if (pins & M6502_RW) {
        M6502_SET_DATA(pins, mem[addr]);
    }
    else {
        mem[addr] = M6502_GET_DATA(pins);
    }
}

static bool pins_sync(void) {
    return (M6502_SYNC|M6502_RW) == (pins & (M6502_SYNC|M6502_RW));
}

static bool pins_read(void) {
    return M6502_RW == (pins & (M6502_SYNC|M6502_RW));
}

static bool pins_write(void) {
    return 0 == (pins & (M6502_SYNC|M6502_RW));
}

static bool pins_addr(uint16_t a) {
    return M6502_GET_ADDR(pins) == a;
}

static void w16(uint16_t addr, uint16_t val) {
    mem[addr] = (uint8_t) val;
    mem[(addr + 1) & 0xFFFF] = (uint8_t) (val>>8);
}

static void copy(uint16_t addr, const uint8_t* bytes, size_t num_bytes) {
    assert((addr + num_bytes) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num_bytes);
}

static void skip(int num_ticks) {
    for (int i = 0; i < num_ticks; i++) {
        tick();
    }
}

static void init(uint16_t start_addr, const uint8_t* bytes, size_t num_bytes) {
    memset(mem, 0, sizeof(mem));
    pins = m6502_init(&cpu, &(m6502_desc_t){0});
    copy(start_addr, bytes, num_bytes);
    // ISR start address
    w16(0xFFFA, 0x0100); // NMI
    w16(0xFFFE, 0x0100); // IRQ
    // run through reset sequence
    skip(7);
    cpu.S = 0xBD;
}

// test that an NMI is taken at the last possible clock cycle
UTEST(m6502_int, NMI_taken) {
    uint8_t prog[] = {
        0xEA, 0xEA, 0xEA, 0xEA, // 4x NOP
    };
    uint8_t isr[] = {
        0x40,   // RTS
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0100, isr, sizeof(isr));
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());

    // NOP with NMI pin set in first cycle, interrupt should be triggered at end of instruction
    pins |= M6502_NMI;
    tick(); T(pins_read());
    pins &= ~M6502_NMI;
    tick(); T(pins_sync());
    
    // interrupt handling should start here
    tick(); T(pins_read());
    tick(); T(pins_write()); T(pins_addr(0x01BD));
    tick(); T(pins_write()); T(pins_addr(0x01BC));
    tick(); T(pins_write()); T(pins_addr(0x01BB));
    tick(); T(pins_read());  T(pins_addr(0xFFFA));
    tick(); T(pins_read());  T(pins_addr(0xFFFB));
    tick(); T(pins_sync());  T(pins_addr(0x0100));
    
    // RTI
    tick(); T(pins_read()); T(pins_addr(0x0101));
    tick(); T(pins_read()); T(pins_addr(0x01BA));
    tick(); T(pins_read()); T(pins_addr(0x01BB));
    tick(); T(pins_read()); T(pins_addr(0x01BC));
    tick(); T(pins_read()); T(pins_addr(0x01BD));
    tick(); T(pins_sync()); T(pins_addr(0x0002));
}

// test that interrupt is delayed to next instruction when NMI is set in last tick
UTEST(m6502_int, NMI_delayed) {
    uint8_t prog[] = {
        0xEA, 0xEA, 0xEA, 0xEA, // 4x NOP
    };
    uint8_t isr[] = {
        0x40,   // RTS
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0100, isr, sizeof(isr));
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());

    // NOP with NMI pin set in last clock cycle
    tick(); T(pins_read());
    pins |= M6502_NMI;
    tick(); T(pins_sync());
    pins &= ~M6502_NMI;
    
    // a regular NOP should follow
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // ...and NMI handling should start here
    tick(); T(pins_read());
    tick(); T(pins_write()); T(pins_addr(0x01BD));
    tick(); T(pins_write()); T(pins_addr(0x01BC));
    tick(); T(pins_write()); T(pins_addr(0x01BB));
    tick(); T(pins_read());  T(pins_addr(0xFFFA));
    tick(); T(pins_read());  T(pins_addr(0xFFFB));
    tick(); T(pins_sync());  T(pins_addr(0x0100));
    
    // RTI
    tick(); T(pins_read()); T(pins_addr(0x0101));
    tick(); T(pins_read()); T(pins_addr(0x01BA));
    tick(); T(pins_read()); T(pins_addr(0x01BB));
    tick(); T(pins_read()); T(pins_addr(0x01BC));
    tick(); T(pins_read()); T(pins_addr(0x01BD));
    tick(); T(pins_sync()); T(pins_addr(0x0003));
}

// test that the IRQ pin is sampled at the right clock cycle
UTEST(m6502_int, IRQ_taken) {
    uint8_t prog[] = {
        0x58, 0xEA, 0xEA, 0xEA, 0xEA, // CLI + 4x NOP
    };
    uint8_t isr[] = {
        0x40,   // RTS
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0100, isr, sizeof(isr));

    // CLI
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // NOP, set IRQ in first clock cycle
    pins |= M6502_IRQ;
    tick(); T(pins_read());
    pins &= ~M6502_IRQ;
    tick(); T(pins_sync());
    
    // interrupt handling starts
    tick(); T(pins_read());
    tick(); T(pins_write()); T(pins_addr(0x01BD));
    tick(); T(pins_write()); T(pins_addr(0x01BC));
    tick(); T(pins_write()); T(pins_addr(0x01BB));
    tick(); T(pins_read());  T(pins_addr(0xFFFE));
    tick(); T(pins_read());  T(pins_addr(0xFFFF));
    tick(); T(pins_sync());  T(pins_addr(0x0100));
    
    // RTI
    tick(); T(pins_read()); T(pins_addr(0x0101));
    tick(); T(pins_read()); T(pins_addr(0x01BA));
    tick(); T(pins_read()); T(pins_addr(0x01BB));
    tick(); T(pins_read()); T(pins_addr(0x01BC));
    tick(); T(pins_read()); T(pins_addr(0x01BD));
    tick(); T(pins_sync()); T(pins_addr(0x0003));
}

// test that IRQ is not taken when IRQ pin is one clock cycle off
UTEST(m6502_int, IRQ_not_taken) {
    uint8_t prog[] = {
        0x58, 0xEA, 0xEA, 0xEA, 0xEA, // CLI + 4x NOP
    };
    uint8_t isr[] = {
        0x40,   // RTS
    };
    init(0x0000, prog, sizeof(prog));
    copy(0x0100, isr, sizeof(isr));

    // CLI
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // NOP
    tick(); T(pins_read());
    pins |= M6502_IRQ;
    tick(); T(pins_sync());
    pins &= ~M6502_IRQ;
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());
    
    // NOP
    tick(); T(pins_read());
    tick(); T(pins_sync());
}

UTEST_MAIN()
