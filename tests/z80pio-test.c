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
    T(pio.reset_active);
    T(0 == (pio.PORT[Z80PIO_PORT_A].int_control &= Z80PIO_INTCTRL_EI));
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL;
    Z80PIO_SET_DATA(pio.PINS, 0xEE);
    z80pio_tick(&pio);
    T(!pio.reset_active);
    T(pio.PORT[Z80PIO_PORT_A].int_vector == 0xEE);
    T(pio.PORT[Z80PIO_PORT_A].int_control &= Z80PIO_INTCTRL_EI);

    /* write interrupt vector 0xCC for port B */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL|Z80PIO_BASEL;
    Z80PIO_SET_DATA(pio.PINS, 0xCC);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_B].int_vector == 0xCC);
    T(pio.PORT[Z80PIO_PORT_B].int_control &= Z80PIO_INTCTRL_EI);

    /* set port A to output */
    T(0 == (pio.PINS & Z80PIO_ARDY));
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL;
    Z80PIO_SET_DATA(pio.PINS, (Z80PIO_MODE_OUTPUT<<6)|0x0F);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_A].mode == Z80PIO_MODE_OUTPUT);
    T(pio.PINS & Z80PIO_ARDY);

    /* set port B to input */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL|Z80PIO_BASEL;
    Z80PIO_SET_DATA(pio.PINS, (Z80PIO_MODE_INPUT<<6)|0x0F);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_B].mode == Z80PIO_MODE_INPUT);

    /* set port A to bidirectional */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL;
    Z80PIO_SET_DATA(pio.PINS, (Z80PIO_MODE_BIDIRECTIONAL<<6)|0x0F);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_A].mode == Z80PIO_MODE_BIDIRECTIONAL);

    /* set port A to mode control (plus followup io_select mask) */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL;
    Z80PIO_SET_DATA(pio.PINS, (Z80PIO_MODE_BITCONTROL<<6)|0x0F);
    z80pio_tick(&pio);
    T(!pio.PORT[Z80PIO_PORT_A].int_enabled);
    T(pio.PORT[Z80PIO_PORT_A].mode == Z80PIO_MODE_BITCONTROL);
    Z80PIO_SET_DATA(pio.PINS, 0xAA);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_A].int_enabled);
    T(pio.PORT[Z80PIO_PORT_A].io_select == 0xAA);

    /* set port B interrupt control word (with interrupt control mask following) */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL|Z80PIO_BASEL;
    Z80PIO_SET_DATA(pio.PINS, (Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS)|0x07);
    z80pio_tick(&pio);
    T(!pio.PORT[Z80PIO_PORT_B].int_enabled);
    T(pio.PORT[Z80PIO_PORT_B].int_control == (Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS));
    Z80PIO_SET_DATA(pio.PINS, 0x23);
    z80pio_tick(&pio);
    T(!pio.PORT[Z80PIO_PORT_B].int_enabled);
    T(pio.PORT[Z80PIO_PORT_B].int_mask == 0x23);

    /* enable interrupts on port B */
    pio.PINS = Z80PIO_IORQ|Z80PIO_CE|Z80PIO_CDSEL|Z80PIO_BASEL;
    Z80PIO_SET_DATA(pio.PINS, Z80PIO_INTCTRL_EI|0x03);
    z80pio_tick(&pio);
    T(pio.PORT[Z80PIO_PORT_B].int_enabled);
    T(pio.PORT[Z80PIO_PORT_B].int_control == (Z80PIO_INTCTRL_EI|Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS));
}

int main() {
    test_write_control();
    return 0;
}
