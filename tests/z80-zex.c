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
#include <inttypes.h>

enum {
    mem_size = 1<<16,
    mem_mask = mem_size-1,
    output_size = 1<<16
};

static uint8_t mem[mem_size];
static int out_pos = 0;
static char output[output_size];

static void put_char(char c) {
    if (out_pos < output_size) {
        output[out_pos++] = c;
    }
    putchar(c);
}

/* Z80 tick callback */
static uint64_t tick(int num, uint64_t pins, void* user_data) {
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
        }
        else if (pins & Z80_WR) {
            mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
        }
    }
    return pins;
}

/* emulate character and string output CP/M system calls */
static bool cpm_bdos(z80_t* cpu) {
    bool retval = true;
    if (2 == cpu->state.C) {
        // output character in register E
        put_char(cpu->state.E);
    }
    else if (9 == cpu->state.C) {
        // output $-terminated string pointed to by register DE */
        uint8_t c;
        uint16_t addr = cpu->state.DE;
        while ((c = mem[addr++ & mem_mask]) != '$') {
            put_char(c);
        }
    }
    else {
        printf("Unhandled CP/M system call: %d\n", cpu->state.C);
        retval = false;
    }
    // emulate a RET
    uint8_t z = mem[cpu->state.SP++];
    uint8_t w = mem[cpu->state.SP++];
    cpu->state.PC = cpu->state.WZ = (w<<8)|z;
    return retval;
}

/* run CPU through the configured test (ZEXDOC or ZEXALL) */
static bool run_test(z80_t* cpu, const char* name) {
    bool running = true;
    uint64_t ticks = 0;
    uint64_t start_time = stm_now();
    while (running) {
        /* run for a lot of ticks or until HALT is encountered */
        ticks += z80_exec(cpu, (1<<30));
        /* check for BDOS call */
        if (5 == cpu->state.PC) {
            if (!cpm_bdos(cpu)) {
                running = false;
            }
        }
        else if (0 == cpu->state.PC) {
            running = false;
        }
        cpu->pins &= ~Z80_HALT;
    }
    double dur = stm_sec(stm_since(start_time));
    printf("\n%s: %"PRIu64" cycles in %.3fsecs (%.2f MHz)\n", name, ticks, dur, (ticks/dur)/1000000.0);

    /* check if an error occurred */
    if (output_size > 0) {
        output[output_size-1] = 0;
        if (strstr((const char*)output, "ERROR")) {
            return false;
        }
        else {
            printf("\n\n ALL %s TESTS PASSED!\n", name);
        }
    }
    return true;
}

/* run the ZEXDOC test */
static bool zexdoc() {
    memset(output, 0, sizeof(output));
    memset(mem, 0, sizeof(mem));
    memcpy(&mem[0x0100], dump_zexdoc, sizeof(dump_zexdoc));
    z80_t cpu;
    z80_init(&cpu, &(z80_desc_t){ .tick_cb=tick });
    cpu.state.SP = 0xF000;
    cpu.state.PC = 0x0100;
    /* trap when reaching address 0x0000 or 0x0005 */
    z80_set_trap(&cpu, 0, 0x0000);
    z80_set_trap(&cpu, 1, 0x0005);
    return run_test(&cpu, "ZEXDOC");
}

/* run the ZEXALL test */
static bool zexall() {
    memset(output, 0, sizeof(output));
    memset(mem, 0, sizeof(mem));
    memcpy(&mem[0x0100], dump_zexall, sizeof(dump_zexall));
    z80_t cpu;
    z80_init(&cpu, &(z80_desc_t){ .tick_cb=tick });
    cpu.state.SP = 0xF000;
    cpu.state.PC = 0x0100;
    /* trap when reaching address 0x0000 or 0x0005 */
    z80_set_trap(&cpu, 0, 0x0000);
    z80_set_trap(&cpu, 1, 0x0005);
    return run_test(&cpu, "ZEXALL");
}

int main() {
    stm_setup();
    if (!zexdoc()) {
        return 10;
    }
    if (!zexall()) {
        return 10;
    }
    return 0;
}
