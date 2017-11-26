//------------------------------------------------------------------------------
//  z80-zes.c
//
//  Runs Frank Cringle's zexdoc and zexall test through the Z80 emu. Provide
//  a minimal CP/M environment to make these work.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"
#include "zex-dump.h"
#include <stdio.h>

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
static uint64_t tick(uint64_t pins) {
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[Z80_ADDR(pins)]);
        }
        else if (pins & Z80_WR) {
            mem[Z80_ADDR(pins)] = Z80_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, 0xFF);
        }
        else if (pins & Z80_WR) {
            /* IO write, do nothing */
        }
    }
    return pins;
}

/* emulate character and string output CP/M system calls */
static bool cpm_bdos(z80* cpu) {
    bool retval = true;
    if (2 == cpu->C) {
        // output character in register E
        put_char(cpu->E);
    }
    else if (9 == cpu->C) {
        // output $-terminated string pointed to by register DE */
        uint8_t c;
        uint16_t addr = cpu->DE;
        while ((c = mem[addr++ & mem_mask]) != '$') {
            put_char(c);
        }
    }
    else {
        printf("Unhandled CP/M system call: %d\n", cpu->C);
        retval = false;
    }
    // emulate a RET
    uint8_t z = mem[cpu->SP++];
    uint8_t w = mem[cpu->SP++];
    cpu->PC = cpu->WZ = (w<<8)|z;
    return retval;
}

/* run CPU through the configured test (ZEXDOC or ZEXALL) */
static bool run_test(z80* cpu, const char* name) {
    bool running = true;
    uint64_t ticks = 0;
    uint64_t ops = 0;
    while (running) {
        ticks += z80_step(cpu);
        ops++;
        /* check for BDOS call */
        if (5 == cpu->PC) {
            if (!cpm_bdos(cpu)) {
                running = false;
            }
        }
        else if (0 == cpu->PC) {
            running = false;
        }
    }

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
    z80 cpu;
    z80_init(&cpu, &(z80_desc){ .tick_func = tick });
    cpu.SP = 0xF000;
    cpu.PC = 0x0100;
    return run_test(&cpu, "ZEXDOC");
}

/* run the ZEXALL test */
static bool zexall() {
    memset(output, 0, sizeof(output));
    memset(mem, 0, sizeof(mem));
    memcpy(&mem[0x0100], dump_zexall, sizeof(dump_zexall));
    z80 cpu;
    z80_init(&cpu, &(z80_desc){ .tick_func = tick });
    cpu.SP = 0xF000;
    cpu.PC = 0x0100;
    return run_test(&cpu, "ZEXALL");
}

int main() {
    if (!zexdoc()) {
        return 10;
    }
    if (!zexall()) {
        return 10;
    }
    return 0;
}
