//------------------------------------------------------------------------------
//  z80-zex.c
//
//  Runs Frank Cringle's zexdoc and zexall test through the Z80 emu. Provide
//  a minimal CP/M environment to make these work.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"
#define SOKOL_IMPL
#include "sokol_time.h"
#include "roms/zex-dump.h"
#include <stdio.h>
#include <inttypes.h> // PRIu64

#define MEM_SIZE (1<<16)
#define MEM_MASK (MEM_SIZE-1)
#define OUTPUT_SIZE (1<<16)

static struct {
    z80_t cpu;
    uint8_t mem[MEM_SIZE];
    int out_pos;
    char output[OUTPUT_SIZE];
} state;

static void put_char(char c) {
    if (state.out_pos < OUTPUT_SIZE) {
        state.output[state.out_pos++] = c;
    }
    putchar(c);
}

static uint64_t tick(uint64_t pins) {
    pins = z80_tick(&state.cpu, pins);
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, state.mem[Z80_GET_ADDR(pins)]);
        } else if (pins & Z80_WR) {
            state.mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
        }
    }
    return pins;
}

// emulate character and string output CP/M system calls
static bool cpm_bdos(void) {
    bool retval = true;
    if (2 == state.cpu.c) {
        // output character in register E
        put_char(state.cpu.e);
    } else if (9 == state.cpu.c) {
        // output $-terminated string pointed to by register DE
        uint8_t c;
        uint16_t addr = state.cpu.de;
        while ((c = state.mem[addr++ & MEM_MASK]) != '$') {
            put_char(c);
        }
    } else {
        printf("Unhandled CP/M system call: %d\n", state.cpu.c);
        retval = false;
    }
    // emulate a RET
    uint8_t z = state.mem[state.cpu.sp++];
    uint8_t w = state.mem[state.cpu.sp++];
    state.cpu.wz = (w<<8) | z;
    state.cpu.pc = state.cpu.wz;
    return retval;
}

static bool run_test(const char* name, uint8_t* prog, size_t prog_num_bytes) {
    bool running = true;
    uint64_t ticks = 0;

    memset(state.output, 0, sizeof(state.output));
    memset(state.mem, 0, sizeof(state.mem));
    memcpy(&state.mem[0x0100], prog, prog_num_bytes);
    uint64_t pins = z80_init(&state.cpu);
    state.cpu.sp = 0xF000;
    z80_prefetch(&state.cpu, 0x0100);
    uint64_t start_time = stm_now();
    while (running) {
        pins = tick(pins);
        ticks++;
        // check for BDOS call
        if (state.cpu.pc == 5) {
            running = cpm_bdos();
        } else if (state.cpu.pc == 0) {
            running = false;
        }
    }
    double dur = stm_sec(stm_since(start_time));
    printf("\n%s: %"PRIu64" cycles in %.3fsecs (%.2f MHz)\n", name, ticks, dur, (ticks/dur)/1000000.0);

    /* check if an error occurred */
    if (state.out_pos > 0) {
        state.output[state.out_pos-1] = 0;
        if (strstr((const char*)state.output, "ERROR")) {
            return false;
        } else {
            printf("\n\n ALL %s TESTS PASSED!\n", name);
        }
    }
    return true;
}

int main() {
    stm_setup();
    if (!run_test("ZEXALL", dump_zexall_com, sizeof(dump_zexall_com))) {
        return 10;
    }
    return 0;
}
