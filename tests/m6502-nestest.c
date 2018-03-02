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

    /* initialize the CPU */
    m6502_init(&cpu, &(m6502_desc_t){
        .tick_cb = tick,
        .bcd_disabled = true
    });
    m6502_reset(&cpu);
    cpu.state.PC = 0xC000;

    /* run the test */
    int i = 0;
    while (cpu.state.PC != 0xC66E) {
        cpu_state* state = &state_table[i++];
        if ((cpu.state.PC != state->PC) ||
            (cpu.state.A  != state->A) ||
            (cpu.state.X  != state->X) ||
            (cpu.state.Y  != state->Y) ||
            (cpu.state.P  != state->P) ||
            (cpu.state.S  != state->S))
        {
            printf("### NESTEST failed at pos %d, PC=0x%04X: %s", i, cpu.state.PC, state->desc);
            assert(cpu.state.PC == state->PC);
            assert(cpu.state.A == state->A);
            assert(cpu.state.X == state->X);
            assert(cpu.state.Y == state->Y);
            assert(cpu.state.P == state->P);
            assert(cpu.state.S == state->S);
            return 10;
        }
        m6502_exec(&cpu, 0);
    }
    puts(">>> NESTEST FINISHED SUCCESSFULLY");
    return 0;
}
