//------------------------------------------------------------------------------
//  z80pio_test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80pio.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void test_write_control() {
    z80pio pio;
    z80pio_init(&pio);

    /* write interrupt vector 0xEE for port A */
    Z80PIO_ON(pio.PINS, Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL);
    Z80PIO_SET_DATA(pio.PINS, 0xEE);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_A].int_vector == 0xEE);

    /* write interrupt vector 0xCC for port B */
    Z80PIO_ON(pio.PINS, Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL|Z80PIO_BASEL);
    Z80PIO_SET_DATA(pio.PINS, 0xCC);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_B].int_vector == 0xCC);
}

int main() {
    test_write_control();
    return 0;
}