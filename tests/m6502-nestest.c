//------------------------------------------------------------------------------
//  m6502-nestest.c
//
//  Tests CPU state after documented instructions, no BCD mode.
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

uint64_t tick(uint64_t pins) {
    pins = m6502_tick(&cpu, pins);
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
       the nestest fileformat has a 16-byte header,
       patch the test start address into the RESET vector
    */
    dump_nestest_nes[16 + 0x3FFC] = 0x00;
    dump_nestest_nes[16 + 0x3FFD] = 0xC0;
    mem_map_rom(&mem, 0, 0x8000, 0x4000, &(dump_nestest_nes[16]));
    mem_map_rom(&mem, 0, 0xC000, 0x4000, &(dump_nestest_nes[16]));

    /* initialize the CPU */
    uint64_t pins = m6502_init(&cpu, &(m6502_desc_t){
        .bcd_disabled = true,
    });
    /* set RESET vector and run through RESET sequence */
    for (int i = 0; i < 7; i++) {
        pins = tick(pins);
    }
    cpu.P &= ~M6502_ZF;

    /* run the test */
    int num_tests = sizeof(state_table) / sizeof(cpu_state);
    for (int i = 0; i < num_tests; i++) {
        cpu_state* state = &state_table[i];
        test(state->desc);
        T(cpu.PC == state->PC);
        T(cpu.A  == state->A);
        T(cpu.X  == state->X);
        T(cpu.Y  == state->Y);
        T((cpu.P & ~(M6502_XF|M6502_BF)) == (state->P & ~(M6502_XF|M6502_BF)));
        T(cpu.S == state->S);
        if (test_failed()) {
            printf("### NESTEST failed at pos %d, PC=0x%04X: %s\n", i, cpu.PC, state->desc);
        }
        do {
            pins = tick(pins);
        } while (0 == (pins & M6502_SYNC));
    }
    return test_end();
}
