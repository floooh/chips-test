//------------------------------------------------------------------------------
//  z80-fuse.c
//
//  Run the FUSE ZX Spectrum emulator CPU test:
//  https://github.com/tom-seddon/fuse-emulator-code/tree/master/fuse/z80/tests
//
//  NOTE: FUSE doesn't always agree with ZEXALL about the state of the 
//  undocumented XF and YF flag bits, maybe the two tests are based
//  on different Z80 revisions. This test ignores the XF and YF flags
//  for instructions where ZEXALL and FUSE disagree.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// CPU state
typedef struct {
    uint16_t af, bc, de, hl;
    uint16_t af_, bc_, de_, hl_;
    uint16_t ix, iy, sp, pc;
    uint8_t i, r, iff1, iff2, im;
    uint8_t halted;
    int ticks;
} fuse_state_t;

// memory chunk
#define FUSE_MAX_BYTES (32)
typedef struct {
    int num_bytes;
    uint16_t addr;
    uint8_t bytes[FUSE_MAX_BYTES];
} fuse_mem_t;

// a result event
typedef enum {
    EVENT_NONE,
    EVENT_MR,     /* memory read */
    EVENT_MW,     /* memory write */
    EVENT_PR,     /* port read */
    EVENT_PW,     /* port write */
} fuse_eventtype_t;

typedef struct {
    int tick;
    fuse_eventtype_t type;
    uint16_t addr;
    uint8_t data;
} fuse_event_t;

// a complete test (used for input, output, expected)
#define FUSE_MAX_EVENTS (128)
#define FUSE_MAX_MEMCHUNKS (8)
typedef struct {
    const char* desc;
    uint8_t num_events;
    uint8_t num_chunks;
    fuse_state_t state;
    fuse_event_t events[FUSE_MAX_EVENTS];
    fuse_mem_t chunks[FUSE_MAX_MEMCHUNKS];
} fuse_test_t;

#include "fuse/fuse.h"

static uint8_t mem[1<<16];

/* don't test the XF/YF flags in the indirect BIT test instructions,
    since FUSE handles those wrong
*/
static const char* xfyf_blacklist[] = {
    "cb46",     // BIT 0,(HL)
    "cb4e",     // BIT 1,(HL)
    "cb56",     // BIT 2,(HL)
    "cb5e",     // BIT 3,(HL)
    "cb66",     // BIT 4,(HL)
    "cb6e",     // BIT 5,(HL)
    "cb76",     // BIT 6,(HL)
    "cb7e",     // BIT 7,(HL)
    0
};

static bool test_xfyf(const char* name) {
    int i = 0;
    while (xfyf_blacklist[i]) {
        if (0 == strcmp(xfyf_blacklist[i], name)) {
            return false;
        }
        i++;
    }
    return true;
}

static uint64_t tick(z80_t* cpu, uint64_t pins) {
    pins = z80_tick(cpu, pins);
    if (pins & Z80_MREQ) {
        uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[addr]);
        }
        else if (pins & Z80_WR) {
            mem[addr] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            uint16_t port = Z80_GET_ADDR(pins);
            Z80_SET_DATA(pins, port >> 8);
        }
    }
    return pins;
}

static bool run_test(fuse_test_t* inp, fuse_test_t* exp) {

    // prepare CPU and memory with test input data (same initial state as coretest.c in FUSE
    for (int i = 0; i < 0x10000; i += 4) {
        mem[i+0] = 0xDE; mem[i+1] = 0xAD;
        mem[i+2] = 0xBE; mem[i+3] = 0xEF;
    }
    z80_t cpu;
    z80_init(&cpu);
    cpu.af = inp->state.af;
    cpu.bc = inp->state.bc;
    cpu.de = inp->state.de;
    cpu.hl = inp->state.hl;
    cpu.af2 = inp->state.af_;
    cpu.bc2 = inp->state.bc_;
    cpu.de2 = inp->state.de_;
    cpu.hl2 = inp->state.hl_;
    cpu.ix = inp->state.ix;
    cpu.iy = inp->state.iy;
    cpu.sp = inp->state.sp;
    cpu.pc = inp->state.pc;
    cpu.i = inp->state.i;
    cpu.r = inp->state.r;
    cpu.iff1 = 0 != inp->state.iff1;
    cpu.iff2 = 0 != inp->state.iff2;
    cpu.im = inp->state.im;
    for (int i = 0; i < inp->num_chunks; i++) {
        uint16_t addr = inp->chunks[i].addr;
        int num_bytes = inp->chunks[i].num_bytes;
        for (int bi = 0; bi < num_bytes; bi++) {
            mem[addr++ & 0xFFFF] = inp->chunks[i].bytes[bi];
        }
    }

    // run CPU for at least N ticks or instruction completion
    uint64_t pins = z80_prefetch(&cpu, cpu.pc);
    pins = tick(&cpu, pins);
    int num_ticks = 0;
    do {
       pins = tick(&cpu, pins);
       num_ticks++;
    } while ((num_ticks < (inp->state.ticks)) || !z80_opdone(&cpu));

    // compare result against expected state
    bool ok = true;
    uint16_t af_mask = 0xFFFF;
    if (!test_xfyf(inp->desc)) {
        af_mask &= ~(Z80_XF|Z80_YF);
    }
    if (num_ticks != exp->state.ticks) {
        printf("\n  %s: TICKS: %d (expected %d)", inp->desc, num_ticks, exp->state.ticks);
        ok = false;
    }
    if ((exp->state.af & af_mask) != (cpu.af & af_mask)) {
        printf("\n  %s: AF: 0x%04X (expected 0x%04X)", inp->desc, cpu.af&af_mask, exp->state.af&af_mask);
        ok = false;
    }
    if (exp->state.bc != cpu.bc) {
        printf("\n  %s: BC: 0x%04X (expected 0x%04X)", inp->desc, cpu.bc, exp->state.bc);
        ok = false;
    }
    if (exp->state.de != cpu.de) {
        printf("\n  %s: DE: 0x%04X (expected 0x%04X)", inp->desc, cpu.de, exp->state.de);
        ok = false;
    }
    if (exp->state.hl != cpu.hl) {
        printf("\n  %s: HL: 0x%04X (expected 0x%04X)", inp->desc, cpu.hl, exp->state.hl);
        ok = false;
    }
    if (exp->state.af_ != cpu.af2) {
        printf("\n  %s: AF': 0x%04X (expected 0x%04X)", inp->desc, cpu.af2, exp->state.af_);
        ok = false;
    }
    if (exp->state.bc_ != cpu.bc2) {
        printf("\n  %s: BC': 0x%04X (expected 0x%04X)", inp->desc, cpu.bc2, exp->state.bc_);
        ok = false;
    }
    if (exp->state.de_ != cpu.de2) {
        printf("\n  %s: DE': 0x%04X (expected 0x%04X)", inp->desc, cpu.de2, exp->state.de_);
        ok = false;
    }
    if (exp->state.hl_ != cpu.hl2) {
        printf("\n  %s: HL': 0x%04X (expected 0x%04X)", inp->desc, cpu.hl2, exp->state.hl_);
        ok = false;
    }
    if (exp->state.ix != cpu.ix) {
        printf("\n  %s: IX: 0x%04X (expected 0x%04X)", inp->desc, cpu.ix, exp->state.ix);
        ok = false;
    }
    if (exp->state.iy != cpu.iy) {
        printf("\n  %s: IY: 0x%04X (expected 0x%04X)", inp->desc, cpu.iy, exp->state.iy);
        ok = false;
    }
    if (exp->state.sp != cpu.sp) {
        printf("\n  %s: SP: 0x%04X (expected 0x%04X)", inp->desc, cpu.sp, exp->state.sp);
        ok = false;
    }
    if (exp->state.pc != (cpu.pc-1)) {
        printf("\n  %s: PC: 0x%04X (expected 0x%04X)", inp->desc, cpu.pc, exp->state.pc);
        ok = false;
    }
    if (exp->state.i != cpu.i) {
        printf("\n  %s: I: 0x%02X (expected 0x%02X)", inp->desc, cpu.i, exp->state.i);
        ok = false;
    }
    if (exp->state.r != cpu.r) {
        printf("\n  %s: R: 0x%02X (expected 0x%02X)", inp->desc, cpu.r, exp->state.r);
        ok = false;
    }
    if (exp->state.iff1 != cpu.iff1) {
        printf("\n  %s: IFF1: %s (expected %s)", inp->desc, cpu.iff1?"true":"false", exp->state.iff1?"true":"false");
        ok = false;
    }
    if (exp->state.iff2 != cpu.iff2) {
        printf("\n  %s: IFF2: %s (expected %s)", inp->desc, cpu.iff2?"true":"false", exp->state.iff2?"true":"false");
        ok = false;
    }
    if (exp->state.im != cpu.im) {
        printf("\n  %s: IM: 0x%02X (expected 0x%02X)", inp->desc, cpu.im, exp->state.im);
        ok = false;
    }
    if ((0 != exp->state.halted) != (0 != (pins & Z80_HALT))) {
        printf("\n  %s: HALT: %s (expected %s)", inp->desc, (pins&Z80_HALT)?"true":"false", exp->state.halted?"true":"false");
        ok = false;
    }
    // check memory content
    for (int i = 0; i < exp->num_chunks; i++) {
        fuse_mem_t* chunk = &(exp->chunks[i]);
        uint16_t addr = chunk->addr;
        for (int bi = 0; bi < chunk->num_bytes; bi++) {
            if (chunk->bytes[bi] != mem[(addr+bi) & 0xFFFF]) {
                printf("\n  %s: BYTE AT 0x%04X IS 0x%02X (expected 0x%02X)", inp->desc,
                    (addr+bi) & 0xFFFF,
                    mem[(addr+bi) & 0xFFFF],
                    chunk->bytes[bi]);
                ok = false;
            }
        }
    }
    return ok;
}

int main() {
    assert(fuse_expected_num == fuse_input_num);
    printf("FUSE Z80 TEST\n");
    int num_failed = 0;
    for (int i = 0; i < fuse_input_num; i++) {
        if (!run_test(&fuse_input[i], &fuse_expected[i])) {
            num_failed++;
        }
    }
    printf("\n");
    if (0 == num_failed) {
        printf("All tests succeeded.\n");
        return 0;
    }
    else {
        printf("%d TESTS FAILED!\n", num_failed);
        return 10;
    }
}
