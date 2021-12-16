//------------------------------------------------------------------------------
//  z80pio_test.c
//
//  FIXME:
//  - interrupt handling (e.g. used in KC87 keyboard input)
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/z80pio.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(z80pio, read_write_control) {
    z80pio_t pio;
    z80pio_init(&pio);
    /* write interrupt vector 0xEE for port A */
    T(pio.reset_active);
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, 0xEE);
    T(!pio.reset_active);
    T(pio.port[Z80PIO_PORT_A].int_vector == 0xEE);
    T(pio.port[Z80PIO_PORT_A].int_control & Z80PIO_INTCTRL_EI);

    /* write interrupt vector 0xCC for port B */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, 0xCC);
    T(pio.port[Z80PIO_PORT_B].int_vector == 0xCC);
    T(pio.port[Z80PIO_PORT_B].int_control & Z80PIO_INTCTRL_EI);

    /* set port A to output */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, (Z80PIO_MODE_OUTPUT<<6)|0x0F);
    T(pio.port[Z80PIO_PORT_A].mode == Z80PIO_MODE_OUTPUT);

    /* set port B to input */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, (Z80PIO_MODE_INPUT<<6)|0x0F);
    T(pio.port[Z80PIO_PORT_B].mode == Z80PIO_MODE_INPUT);

    /* set port A to bidirectional */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, (Z80PIO_MODE_BIDIRECTIONAL<<6)|0x0F);
    T(pio.port[Z80PIO_PORT_A].mode == Z80PIO_MODE_BIDIRECTIONAL);

    /* set port A to mode control (plus followup io_select mask) */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, (Z80PIO_MODE_BITCONTROL<<6)|0x0F);
    T(!pio.port[Z80PIO_PORT_A].int_enabled);
    T(pio.port[Z80PIO_PORT_A].mode == Z80PIO_MODE_BITCONTROL);
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, 0xAA);
    T(pio.port[Z80PIO_PORT_A].int_enabled);
    T(pio.port[Z80PIO_PORT_A].io_select == 0xAA);

    /* set port B interrupt control word (with interrupt control mask following) */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, (Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS)|0x07);
    T(!pio.port[Z80PIO_PORT_B].int_enabled);
    T(pio.port[Z80PIO_PORT_B].int_control == (Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS));
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, 0x23);
    T(!pio.port[Z80PIO_PORT_B].int_enabled);
    T(pio.port[Z80PIO_PORT_B].int_mask == 0x23);

    /* enable interrupts on port B */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, Z80PIO_INTCTRL_EI|0x03);
    T(pio.port[Z80PIO_PORT_B].int_enabled);
    T(pio.port[Z80PIO_PORT_B].int_control == (Z80PIO_INTCTRL_EI|Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|Z80PIO_INTCTRL_MASK_FOLLOWS));

    /* write interrupt control word to A and B, 
       and read the control word back, this does not
       seem to be documented anywhere, so we're doing
       the same thing that MAME does.
    */
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_A, Z80PIO_INTCTRL_ANDOR|Z80PIO_INTCTRL_HILO|0x07);
    _z80pio_write_ctrl(&pio, Z80PIO_PORT_B, Z80PIO_INTCTRL_EI|Z80PIO_INTCTRL_ANDOR|0x07);
    uint8_t data = _z80pio_read_ctrl(&pio);
    T(data == 0x4C);
}
