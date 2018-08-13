//------------------------------------------------------------------------------
//  z80-fuse.c
//
//  Run the FUSE ZX Spectrum emulator CPU test:
//  https://github.com/tom-seddon/fuse-emulator-code/tree/master/fuse/z80/tests
//
//  I'm quite sure that FUSE handles the undocumented XF/YF flag bits wrong
//  for the BIT n,(HL), BIT n,(IX+d), BIT n,(IY+d) instructions, since
//  the FUSE Z80 emulation doesn't seem to know about the WZ register.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* CPU state */
typedef struct {
    uint16_t af, bc, de, hl;
    uint16_t af_, bc_, de_, hl_;
    uint16_t ix, iy, sp, pc;
    uint8_t i, r, iff1, iff2, im;
    uint8_t halted;
    int ticks;
} fuse_state_t;

/* memory chunk */
#define FUSE_MAX_BYTES (32)
typedef struct {
    int num_bytes;
    uint16_t addr;
    uint8_t bytes[FUSE_MAX_BYTES];
} fuse_mem_t;

/* a result event */
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

/* a complete test (used for input, output, expected) */
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

uint8_t mem[1<<16];

/* don't test the XF/YF flags in the indirect BIT test instructions,
    since FUSE handles those wrong
*/
const char* xfyf_blacklist[] = {
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

bool test_xfyf(const char* name) {
    int i = 0;
    while (xfyf_blacklist[i]) {
        if (0 == strcmp(xfyf_blacklist[i], name)) {
            return false;
        }
        i++;
    }
    return true;
}

uint64_t cpu_tick(int num, uint64_t pins, void* user_data) {
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
        }
        else if (pins & Z80_WR) {
            mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        // FIXME
    }
    return pins;
}

bool run_test(int test_index, fuse_test_t* inp, fuse_test_t* exp) {
    assert(inp->state.halted == 0);

    /* prepare CPU and memory with test input data (same initial state as coretest.c in FUSE */
    for (int i = 0; i < 0x10000; i += 4) {
        mem[i  ] = 0xDE; mem[i+1] = 0xAD;
        mem[i+2] = 0xBE; mem[i+3] = 0xEF;
    }
    z80_t cpu;
    z80_init(&cpu, &(z80_desc_t){ .tick_cb=cpu_tick });
    z80_set_af(&cpu, inp->state.af);
    z80_set_bc(&cpu, inp->state.bc);
    z80_set_de(&cpu, inp->state.de);
    z80_set_hl(&cpu, inp->state.hl);
    z80_set_af_(&cpu, inp->state.af_);
    z80_set_bc_(&cpu, inp->state.bc_);
    z80_set_de_(&cpu, inp->state.de_);
    z80_set_hl_(&cpu, inp->state.hl_);
    z80_set_ix(&cpu, inp->state.ix);
    z80_set_iy(&cpu, inp->state.iy);
    z80_set_sp(&cpu, inp->state.sp);
    z80_set_pc(&cpu, inp->state.pc);
    z80_set_i(&cpu, inp->state.i);
    z80_set_r(&cpu, inp->state.r);
    z80_set_iff1(&cpu, 0 != inp->state.iff1);
    z80_set_iff2(&cpu, 0 != inp->state.iff2);
    z80_set_im(&cpu, inp->state.im);
    cpu.pins &= ~Z80_HALT;
    for (int i = 0; i < inp->num_chunks; i++) {
        uint16_t addr = inp->chunks[i].addr;
        int num_bytes = inp->chunks[i].num_bytes;
        for (int bi = 0; bi < num_bytes; bi++) {
            mem[addr++ & 0xFFFF] = inp->chunks[i].bytes[bi];
        }
    }

    /* execute N ticks */
    int num_ticks = z80_exec(&cpu, inp->state.ticks);
    while (!z80_opdone(&cpu)) {
        num_ticks += z80_exec(&cpu, 0);
    }

    /* compare result against expected state */
    bool ok = true;
    uint16_t af_mask = 0xFFFF;
    if (!test_xfyf(inp->desc)) {
        af_mask &= ~(Z80_XF|Z80_YF);
    }
    if (num_ticks != exp->state.ticks) {
        printf("\n  %s: TICKS: %d (expected %d)", inp->desc, num_ticks, exp->state.ticks);
        ok = false;
    }
    if ((exp->state.af & af_mask) != (z80_af(&cpu) & af_mask)) {
        printf("\n  %s: AF: 0x%04X (expected 0x%04X)", inp->desc, z80_af(&cpu)&af_mask, exp->state.af&af_mask);
        ok = false;
    }
    if (exp->state.bc != z80_bc(&cpu)) {
        printf("\n  %s: BC: 0x%04X (expected 0x%04X)", inp->desc, z80_bc(&cpu), exp->state.bc);
        ok = false;
    }
    if (exp->state.de != z80_de(&cpu)) {
        printf("\n  %s: DE: 0x%04X (expected 0x%04X)", inp->desc, z80_de(&cpu), exp->state.de);
        ok = false;
    }
    if (exp->state.hl != z80_hl(&cpu)) {
        printf("\n  %s: HL: 0x%04X (expected 0x%04X)", inp->desc, z80_hl(&cpu), exp->state.hl);
        ok = false;
    }
    if (exp->state.af_ != z80_af_(&cpu)) {
        printf("\n  %s: AF': 0x%04X (expected 0x%04X)", inp->desc, z80_af_(&cpu), exp->state.af_);
        ok = false;
    }
    if (exp->state.bc_ != z80_bc_(&cpu)) {
        printf("\n  %s: BC': 0x%04X (expected 0x%04X)", inp->desc, z80_bc_(&cpu), exp->state.bc_);
        ok = false;
    }
    if (exp->state.de_ != z80_de_(&cpu)) {
        printf("\n  %s: DE': 0x%04X (expected 0x%04X)", inp->desc, z80_de_(&cpu), exp->state.de_);
        ok = false;
    }
    if (exp->state.hl_ != z80_hl_(&cpu)) {
        printf("\n  %s: HL': 0x%04X (expected 0x%04X)", inp->desc, z80_hl_(&cpu), exp->state.hl_);
        ok = false;
    }
    if (exp->state.ix != z80_ix(&cpu)) {
        printf("\n  %s: IX: 0x%04X (expected 0x%04X)", inp->desc, z80_ix(&cpu), exp->state.ix);
        ok = false;
    }
    if (exp->state.iy != z80_iy(&cpu)) {
        printf("\n  %s: IY: 0x%04X (expected 0x%04X)", inp->desc, z80_iy(&cpu), exp->state.iy);
        ok = false;
    }
    if (exp->state.sp != z80_sp(&cpu)) {
        printf("\n  %s: SP: 0x%04X (expected 0x%04X)", inp->desc, z80_sp(&cpu), exp->state.sp);
        ok = false;
    }
    if (exp->state.pc != z80_pc(&cpu)) {
        printf("\n  %s: PC: 0x%04X (expected 0x%04X)", inp->desc, z80_pc(&cpu), exp->state.pc);
        ok = false;
    }
    if (exp->state.i != z80_i(&cpu)) {
        printf("\n  %s: I: 0x%02X (expected 0x%02X)", inp->desc, z80_i(&cpu), exp->state.i);
        ok = false;
    }
    if (exp->state.r != z80_r(&cpu)) {
        printf("\n  %s: R: 0x%02X (expected 0x%02X)", inp->desc, z80_r(&cpu), exp->state.r);
        ok = false;
    }
    if (exp->state.iff1 != z80_iff1(&cpu)) {
        printf("\n  %s: IFF1: %s (expected %s)", inp->desc, z80_iff1(&cpu)?"true":"false", exp->state.iff1?"true":"false");
        ok = false;
    }
    if (exp->state.iff2 != z80_iff2(&cpu)) {
        printf("\n  %s: IFF2: %s (expected %s)", inp->desc, z80_iff2(&cpu)?"true":"false", exp->state.iff2?"true":"false");
        ok = false;
    }
    if (exp->state.im != z80_im(&cpu)) {
        printf("\n  %s: IM: 0x%02X (expected 0x%02X)", inp->desc, z80_im(&cpu), exp->state.im);
        ok = false;
    }
    if ((0 != exp->state.halted) != (0 != (cpu.pins & Z80_HALT))) {
        printf("\n  %s: HALT: %s (expected %s)", inp->desc, (cpu.pins&Z80_HALT)?"true":"false", exp->state.halted?"true":"false");
        ok = false;
    }
    /* check memory content */
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
    if (!ok) {
        printf("\n#%d (%s) FAILED!!!\n", test_index, inp->desc);
    }
    return ok;
}

int main() {
    assert(fuse_expected_num == fuse_input_num);
    printf("FUSE EMULATOR Z80 TESTS\n"
           "=======================\n\n");

    int num_succeeded = 0;
    int num_failed = 0;
    for (int i = 0; i < fuse_input_num; i++) {
        if (run_test(i, &fuse_input[i], &fuse_expected[i])) {
            num_succeeded++;
        }
        else {
            num_failed++;
        }
    }
    if (num_failed == 0) {
        printf("\n\nFUSE SUCCESS! (%d out of %d failed)\n", num_failed, fuse_input_num);
        return 0;
    }
    else {
        printf("\n\nFUSE FAILED! (%d out of %d failed)\n", num_failed, fuse_input_num);
        return 10;
    }
}
