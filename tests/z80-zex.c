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

/* Z80 trap callback, checks for addresses 0x000 and 0x005 */
static int trap(uint16_t pc, uint32_t ticks, uint64_t pins, void* user_data) {
    if (pc == 0x0000) {
        return 1;
    }
    else if (pc == 0x0005) {
        return 2;
    }
    else {
        return 0;
    }
}

/* emulate character and string output CP/M system calls */
static bool cpm_bdos(z80_t* cpu) {
    bool retval = true;
    if (2 == z80_c(cpu)) {
        // output character in register E
        put_char(z80_e(cpu));
    }
    else if (9 == z80_c(cpu)) {
        // output $-terminated string pointed to by register DE */
        uint8_t c;
        uint16_t addr = z80_de(cpu);
        while ((c = mem[addr++ & mem_mask]) != '$') {
            put_char(c);
        }
    }
    else {
        printf("Unhandled CP/M system call: %d\n", z80_c(cpu));
        retval = false;
    }
    // emulate a RET
    uint16_t sp = z80_sp(cpu);
    uint8_t z = mem[sp++];
    uint8_t w = mem[sp++];
    z80_set_sp(cpu, sp);
    uint16_t wz = (w<<8)|z;
    z80_set_pc(cpu, wz);
    z80_set_wz(cpu, wz);
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
        const uint16_t pc = z80_pc(cpu);
        if (5 == pc) {
            if (!cpm_bdos(cpu)) {
                running = false;
            }
        }
        else if (0 == pc) {
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
    memcpy(&mem[0x0100], dump_zexdoc_com, sizeof(dump_zexdoc_com));
    z80_t cpu;
    z80_init(&cpu, &(z80_desc_t){ .tick_cb=tick });
    z80_set_sp(&cpu, 0xF000);
    z80_set_pc(&cpu, 0x0100);
    /* trap when reaching address 0x0000 or 0x0005 */
    z80_trap_cb(&cpu, trap, 0);
    return run_test(&cpu, "ZEXDOC");
}

/* run the ZEXALL test */
static bool zexall() {
    memset(output, 0, sizeof(output));
    memset(mem, 0, sizeof(mem));
    memcpy(&mem[0x0100], dump_zexall_com, sizeof(dump_zexall_com));
    z80_t cpu;
    z80_init(&cpu, &(z80_desc_t){ .tick_cb=tick });
    z80_set_sp(&cpu, 0xF000);
    z80_set_pc(&cpu, 0x0100);
    /* trap when reaching address 0x0000 or 0x0005 */
    z80_trap_cb(&cpu, trap, 0);
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

