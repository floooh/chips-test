//------------------------------------------------------------------------------
//  i8255-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/i8255.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

uint8_t values[I8255_NUM_PORTS] = { 0 };

uint8_t in_cb(int port_id) {
    return values[port_id];
}

uint64_t out_cb(int port_id, uint64_t pins, uint8_t data) {
    values[port_id] = data;
    return pins;
}

void test_mode_select() {
    i8255_t ppi;
    i8255_init(&ppi, in_cb, out_cb);
    values[0] = 0x12; values[1] = 0x34; values[2] = 0x45;

    /* configure mode:
        Port A: input
        Port B: output
        Port C: upper half: output, lower half: input
        all on 'basic input/output' (the only one supported)
    */
    uint8_t ctrl =
        I8255_CTRL_CONTROL_MODE |
        I8255_CTRL_A_INPUT |
        I8255_CTRL_B_OUTPUT |
        I8255_CTRL_CHI_OUTPUT |
        I8255_CTRL_CLO_INPUT |
        I8255_CTRL_BCLO_MODE_0 |
        I8255_CTRL_ACHI_MODE_0;
    uint64_t pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_DATA(pins, ctrl);
    uint64_t res_pins = i8255_iorq(&ppi, pins);
    T(pins == res_pins);
    T(values[0] == 0xFF);
    T(values[1] == 0x00);
    T(values[2] == 0x0F);
    T(ppi.control == ctrl);

    /* test reading the control word back */
    pins = I8255_CS|I8255_RD|I8255_A0|I8255_A1;
    pins = i8255_iorq(&ppi, pins);
    uint8_t res_ctrl = I8255_GET_DATA(pins);
    T(res_ctrl == ctrl);
}

void test_bit_set_clear() {
    i8255_t ppi;
    i8255_init(&ppi, in_cb, out_cb);
    values[0] = 0x12; values[1] = 0x34; values[2] = 0x45;

    /* set port C to output */
    uint8_t ctrl =
        I8255_CTRL_CONTROL_MODE |
        I8255_CTRL_CHI_OUTPUT |
        I8255_CTRL_CLO_OUTPUT;
    uint64_t pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_DATA(pins, ctrl);
    i8255_iorq(&ppi, pins);

    /* set bit 3 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_SET|(3<<1);
    pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_DATA(pins, ctrl);
    i8255_iorq(&ppi, pins);
    T(values[2] == (1<<3));   // bit 3 must be set
    /* set bit 5 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_SET|(5<<1);
    pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_DATA(pins, ctrl);
    i8255_iorq(&ppi, pins);
    T(values[2] == ((1<<5)|(1<<3)))     // bit 3 and 5 must be set

    /* clear bit 3 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_RESET|(5<<1);
    pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_DATA(pins, ctrl);
    i8255_iorq(&ppi, pins);
    T(values[2] == (1<<3));   // only bit 3 must be set
}

void test_in_out() {

}

int main() {
    test_mode_select();
    test_bit_set_clear();
    test_in_out();
    return 0;
}
