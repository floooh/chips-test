//------------------------------------------------------------------------------
//  z80pio_test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80pio.h"
#include <stdio.h>

int main() {
    z80pio pio;
    z80pio_init(&pio);
    return 0;
}