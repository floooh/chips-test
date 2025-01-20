//------------------------------------------------------------------------------
//  i8255-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/i8255.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(i8255, mode_select) {
    i8255_t ppi;
    i8255_init(&ppi);

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
    I8255_SET_PA(pins, 0x12);
    I8255_SET_PB(pins, 0x34);
    I8255_SET_PC(pins, 0x45);
    I8255_SET_DATA(pins, ctrl);
    uint64_t res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PA(res_pins) == 0x12);
    T(I8255_GET_PB(res_pins) == 0x00);
    T(I8255_GET_PC(res_pins) == 0x05);
    T(ppi.control == ctrl);

    /* test reading the control word back */
    pins = I8255_CS|I8255_RD|I8255_A0|I8255_A1;
    pins = i8255_tick(&ppi, pins);
    uint8_t res_ctrl = I8255_GET_DATA(pins);
    T(res_ctrl == ctrl);
}

UTEST(i8255, test_bit_set_clear) {
    i8255_t ppi;
    i8255_init(&ppi);

    /* set port C to output */
    uint8_t ctrl =
        I8255_CTRL_CONTROL_MODE |
        I8255_CTRL_CHI_OUTPUT |
        I8255_CTRL_CLO_OUTPUT;
    uint64_t pins = I8255_CS|I8255_WR|I8255_A0|I8255_A1;
    I8255_SET_PA(pins, 0x12);
    I8255_SET_PB(pins, 0x34);
    I8255_SET_PC(pins, 0x45);
    I8255_SET_DATA(pins, ctrl);
    i8255_tick(&ppi, pins);

    /* set bit 3 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_SET|(3<<1);
    I8255_SET_DATA(pins, ctrl);
    uint64_t res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PC(res_pins) == (1<<3));   // bit 3 must be set
    /* set bit 5 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_SET|(5<<1);
    I8255_SET_DATA(pins, ctrl);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PC(res_pins) == ((1<<5)|(1<<3)))     // bit 3 and 5 must be set

    /* clear bit 3 on port C */
    ctrl = I8255_CTRL_CONTROL_BIT|I8255_CTRL_BIT_RESET|(5<<1);
    I8255_SET_DATA(pins, ctrl);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PC(res_pins) == (1<<3));   // only bit 3 must be set
}

UTEST(i8255, in_out) {
    i8255_t ppi;
    i8255_init(&ppi);

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
    I8255_SET_PA(pins, 0x12);
    I8255_SET_PB(pins, 0x34);
    I8255_SET_PC(pins, 0x45);
    I8255_SET_DATA(pins, ctrl);
    i8255_tick(&ppi, pins);

    /* writing to an input port shouldn't do anything */
    pins &= ~(I8255_A1|I8255_A0);
    I8255_SET_PA(pins, 0x12);
    I8255_SET_PB(pins, 0x34);
    I8255_SET_PC(pins, 0x45);
    I8255_SET_DATA(pins, 0x33);
    uint64_t res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PA(res_pins) == 0x12);

    /* writing to an output port (B) should invoke the out callback and set the output latch */
    pins &= ~(I8255_A1|I8255_A0);
    pins |= I8255_A0;
    I8255_SET_DATA(pins, 0xAA);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PB(res_pins) == 0xAA);
    T(ppi.pb.outp == 0xAA);

    /* writing to port C should only affect the 'output half', and set all input half bits */
    pins &= ~(I8255_A1|I8255_A0);
    pins |= I8255_A1;
    I8255_SET_DATA(pins, 0x77);
    I8255_SET_PCLO(pins, 0x0F);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_PC(res_pins) == 0x7F);
    T(ppi.pc.outp == 0x77);

    pins &= ~(I8255_A1|I8255_A0|I8255_RD|I8255_WR);
    pins |= I8255_RD;
    I8255_SET_PA(pins, 0xAB);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_DATA(res_pins) == 0xAB);

    /* reading an output port (B) should return the last output value */
    pins &= ~(I8255_A1|I8255_A0|I8255_RD|I8255_WR);
    pins |= I8255_RD|I8255_A0;
    I8255_SET_PB(pins, 0x23);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_DATA(res_pins) == 0xAA);

    /* reading from mixed input/output return a mixed latched/input result */
    pins &= ~(I8255_A1|I8255_A0|I8255_RD|I8255_WR);
    pins |= I8255_RD|I8255_A1;
    I8255_SET_PC(pins, 0x34);
    res_pins = i8255_tick(&ppi, pins);
    T(I8255_GET_DATA(res_pins) == 0x74);
}
