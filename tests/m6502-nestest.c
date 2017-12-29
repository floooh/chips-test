//------------------------------------------------------------------------------
//  m6502-nestest.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mem.h"
#include <stdio.h>
#include "nestest/dump.h"
#include "nestest/nestestlog.h"

m6502_t cpu;
mem_t mem;
uint8_t ram[0x0800];
uint8_t sram[0x2000];

uint64_t tick(uint64_t pins) {
    const uint16_t addr = M6502_GET_ADDR(pins);
    /* memory-mapped IO range from 2000..401F is ignored */
    /* ignore memory-mapped-io requests */
    if ((addr >= 0x2000) && (addr < 0x4020)) {
        return pins;
    }
    else if (pins & M6502_RW) {
        /* memory read */
        M6502_SET_DATA(pins, mem_rd(&mem, addr));
    }
    else {
        /* memory write */
        mem_wr(&mem, addr, M6502_GET_DATA(pins));
    }
    return pins;
}

int main() {
    puts(">>> RUNNING NESTEST...");

    /* need to implement a minimal NES emulation */
    memset(ram, 0, sizeof(ram));
    memset(sram, 0, sizeof(sram));

    /* RAM 0..7FF is repeated until 1FFF */
    mem_map_ram(&mem, 0, 0x0000, sizeof(ram), ram);
    mem_map_ram(&mem, 0, 0x0800, sizeof(ram), ram);
    mem_map_ram(&mem, 0, 0x1000, sizeof(ram), ram);
    mem_map_ram(&mem, 0, 0x1800, sizeof(ram), ram);

    /* SRAM 6000..8000 */
    mem_map_ram(&mem, 0, 0x6000, sizeof(sram), sram);

    /* map nestest rom (one 16KB bank repeated at 0x8000 and 0xC000)
       the nestest fileformat has a 16-byte header
    */
    mem_map_rom(&mem, 0, 0x8000, 0x4000, &(dump_nestest[16]));
    mem_map_rom(&mem, 0, 0xC000, 0x4000, &(dump_nestest[16]));

    /* initialize the CPU (without BCD arithmetic support) */
    m6502_init(&cpu, tick, false);
    m6502_reset(&cpu);
    cpu.PC = 0xC000;

    /* run the test */
    int i = 0;
    while (cpu.PC != 0xC66E) {
        cpu_state* state = &state_table[i++];
        if ((cpu.PC != state->PC) ||
            (cpu.A  != state->A) ||
            (cpu.X  != state->X) ||
            (cpu.Y  != state->Y) ||
            (cpu.P  != state->P) ||
            (cpu.S  != state->S))
        {
            printf("### NESTEST failed at pos %d, PC=0x%04X: %s", i, cpu.PC, state->desc);
            assert(cpu.PC == state->PC);
            assert(cpu.A == state->A);
            assert(cpu.X == state->X);
            assert(cpu.Y == state->Y);
            assert(cpu.P == state->P);
            assert(cpu.S == state->S);
            return 10;
        }
        m6502_exec(&cpu, 0);
    }
    puts(">>> NESTEST FINISHED SUCCESSFULLY");
    return 0;
}
