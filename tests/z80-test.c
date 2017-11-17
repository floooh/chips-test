//------------------------------------------------------------------------------
//  z80-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80.h"

void tick(z80* cpu, void* ctx) {

}

int main() {
    z80 cpu;
    z80_desc desc = {
        .tick_func = tick,
        .tick_context = 0
    };
    z80_init(&cpu, &desc);

    return 0;
}
