//------------------------------------------------------------------------------
//  m6502-nestest.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mem.h"
#include <stdio.h>
#include "nestest/dump.h"
#include "nestest/nestestlog.h"
#include "test.h"

m6502_t cpu;
mem_t mem;
uint8_t ram[0x0800];
uint8_t sram[0x2000];

uint64_t tick(uint64_t pins, void* user_data) {
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
    test_begin("NES TEST (m6502)");
    test_no_verbose();

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
        .bcd_disabled = true,
        .user_data = 0
    });
    m6502_reset(&cpu);
    cpu.state.PC = 0xC000;

    /* run the test */
    int i = 0;
    while (cpu.state.PC != 0xC66E) {
        cpu_state* state = &state_table[i++];
        test(state->desc);
        T(cpu.state.PC == state->PC);
        T(cpu.state.A  == state->A);
        T(cpu.state.X  == state->X);
        T(cpu.state.Y  == state->Y);
        T(cpu.state.P  == state->P);
        T(cpu.state.S  == state->S);
        if (test_failed()) {
            printf("### NESTEST failed at pos %d, PC=0x%04X: %s\n", i, cpu.state.PC, state->desc);
        }
        m6502_exec(&cpu, 0);
    }
    return test_end();
}
