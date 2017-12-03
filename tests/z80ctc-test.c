//------------------------------------------------------------------------------
//  z80ctc_test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/z80ctc.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void zcto_cb(int chn_id) {
    // FIXME
}

void init(z80ctc* ctc) {
    z80ctc_init(ctc, &(z80ctc_desc){
        .zcto_cb = zcto_cb
    });
}

void test_intvector() {
    z80ctc ctc;
    init(&ctc);
    /* write interrupt vector */
    _z80ctc_write(&ctc, 0, 0xE0);
    T(ctc.chn[0].int_vector == 0xE0);
    T(ctc.chn[1].int_vector == 0xE2);
    T(ctc.chn[2].int_vector == 0xE4);
    T(ctc.chn[3].int_vector == 0xE6);
}

void test_timer() {
    z80ctc ctc;
    init(&ctc);

    /* enable interrupt, mode timer, prescaler 16, trigger-auto, const follows */
    const uint8_t ctrl = Z80CTC_CTRL_EI|Z80CTC_CTRL_MODE_TIMER|
        Z80CTC_CTRL_PRESCALER_16|Z80CTC_CTRL_TRIGGER_AUTO|
        Z80CTC_CTRL_CONST_FOLLOWS|Z80CTC_CTRL_CONTROL;
    _z80ctc_write(&ctc, 1, ctrl);
    T(ctc.chn[1].control == ctrl);
    /* write timer constant */
    _z80ctc_write(&ctc, 1, 10);
    T(0 == (ctc.chn[1].control & Z80CTC_CTRL_CONST_FOLLOWS));
    T(10 == ctc.chn[1].constant);
    T(10 == ctc.chn[1].down_counter);
}

int main() {
    test_intvector();
    test_timer();
    return 0;
}
