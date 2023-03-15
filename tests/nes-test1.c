//------------------------------------------------------------------------------
//  nes-test1.c
//
//  Tests NES state after documented instructions.
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "chips/clk.h"
#include "chips/m6502.h"
#include "chips/r2c02.h"
#include "systems/nes.h"
#include <stdio.h>
#include "nestest/dump.h"
#include "nestest/nestestlog.h"
#include "test.h"

int main() {
    test_begin("NES TEST (NES)");
    test_no_verbose();

    /* initialize the NES */
    nes_t nes;
    nes_init(&nes, &(nes_desc_t){});
    nes_insert_cart(&nes, (chips_range_t){
        .ptr = dump_nestest_nes,
        .size = sizeof(dump_nestest_nes),
    });
    /* patch the test start address into the RESET vector */
    nes.cart.rom[0x3FFC] = 0x00;
    dump_nestest_nes[0x3FFD] = 0xc0;
    nes.cart.rom[0x3FFD] = 0xC0;
     /* set RESET vector and run through RESET sequence */
    uint64_t pins = nes.pins;
    for (int i = 0; i < 7; i++) {
        pins = _nes_tick(&nes, pins);
    }
    nes.cpu.P &= ~M6502_ZF;

    /* run the test */
    int num_tests = sizeof(state_table) / sizeof(cpu_state);
    for (int i = 0; i < num_tests; i++) {
        cpu_state* state = &state_table[i];
        test(state->desc);
        T(nes.cpu.PC == state->PC);
        T(nes.cpu.A  == state->A);
        T(nes.cpu.X  == state->X);
        T(nes.cpu.Y  == state->Y);
        T((nes.cpu.P & ~(M6502_XF|M6502_BF)) == (state->P & ~(M6502_XF|M6502_BF)));
        T(nes.cpu.S == state->S);
        if (test_failed()) {
            printf("### NESTEST failed at pos %d, PC=0x%04X: %s\n", i, nes.cpu.PC, state->desc);
        }
        do {
            pins = _nes_tick(&nes, pins);
        } while (0 == (pins & M6502_SYNC));
    }
    return test_end();
}
