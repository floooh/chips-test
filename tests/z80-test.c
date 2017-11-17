//------------------------------------------------------------------------------
//  z80-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"

uint8_t mem[1<<16] = { 0 };

void tick(z80* cpu) {
    if (cpu->CTRL & Z80_MREQ) {
        if (cpu->CTRL & Z80_RD) {
            /* memory read */
            cpu->DATA = mem[cpu->ADDR];
        }
        else if (cpu->CTRL & Z80_WR) {
            /* memory write */
            mem[cpu->ADDR] = cpu->DATA;
        }
    }
    else if (cpu->CTRL & Z80_IORQ) {
        if (cpu->CTRL & Z80_RD) {
            /* IN */
        }
        else if (cpu->CTRL & Z80_WR) {
            /* OUT */
        }
    }
}

int main() {
    z80 cpu;
    z80_desc desc = {
        .tick_func = tick,
        .tick_context = 0
    };
    z80_init(&cpu, &desc);

    z80_step(&cpu);

    return 0;
}
