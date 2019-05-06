//------------------------------------------------------------------------------
//  i8255-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/i8255.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

static uint8_t values[I8255_NUM_PORTS] = { 0 };

static uint8_t in_cb(int port_id, void* user_data) {
    return values[port_id];
}

static uint64_t out_cb(int port_id, uint64_t pins, uint8_t data, void* user_data) {
    values[port_id] = data;
    return pins;
}

UTEST(i8255, mode_select) {
    i8255_t ppi;
    i8255_init(&ppi, &(i8255_desc_t){
        .in_cb = in_cb,
        .out_cb = out_cb,
        .user_data = 0
    });
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

UTEST(i8255, test_bit_set_clear) {
    i8255_t ppi;
    i8255_init(&ppi, &(i8255_desc_t){
        .in_cb = in_cb,
        .out_cb = out_cb,
        .user_data = 0
    });
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

UTEST(i8255, in_out) {
    i8255_t ppi;
    i8255_init(&ppi, &(i8255_desc_t){
        .in_cb = in_cb,
        .out_cb = out_cb,
        .user_data = 0
    });
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
    i8255_iorq(&ppi, pins);

    /* writing to an input port shouldn't do anything */
    values[0] = 0x12; values[1] = 0x34; values[2] = 0x45;
    pins = I8255_CS|I8255_WR;   /* write to port A */
    I8255_SET_DATA(pins, 0x33);
    i8255_iorq(&ppi, pins);
    T(values[0] == 0x12);

    /* writing to an output port (B) should invoke the out callback and set the output latch */
    pins = I8255_CS|I8255_WR|I8255_A0;
    I8255_SET_DATA(pins, 0xAA);
    i8255_iorq(&ppi, pins);
    T(values[1] == 0xAA);
    T(ppi.output[1] == 0xAA);

    /* writing to port C should only affect the 'output half', and set all input half bits */
    pins = I8255_CS|I8255_WR|I8255_A1;
    I8255_SET_DATA(pins, 0x77);
    i8255_iorq(&ppi, pins);
    T(values[2] == 0x7F);
    T(ppi.output[2] == 0x77);

    /* reading from an input port (A) should ask the input callback */
    values[0] = 0xAB;
    pins = I8255_CS|I8255_RD;
    pins = i8255_iorq(&ppi, pins);
    uint8_t data = I8255_GET_DATA(pins);
    T(data == 0xAB);

    /* reading an output port (B) should return the last output value */
    values[1] = 0x23;
    pins = I8255_CS|I8255_RD|I8255_A0;
    pins = i8255_iorq(&ppi, pins);
    data = I8255_GET_DATA(pins);
    T(data == 0xAA);

    /* reading from mixed input/output return a mixed latched/callback result */
    values[2] = 0x34;
    pins = I8255_CS|I8255_RD|I8255_A1;
    pins = i8255_iorq(&ppi, pins);
    data = I8255_GET_DATA(pins);
    T(data == 0x74);
}
