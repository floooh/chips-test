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

void test_intvector() {
    z80ctc ctc;
    z80ctc_init(&ctc);
    /* write interrupt vector */
    _z80ctc_write(&ctc, 0, 0, 0xE0);
    T(ctc.chn[0].int_vector == 0xE0);
    T(ctc.chn[1].int_vector == 0xE2);
    T(ctc.chn[2].int_vector == 0xE4);
    T(ctc.chn[3].int_vector == 0xE6);
}

void test_timer() {
    z80ctc ctc;
    z80ctc_init(&ctc);
    uint64_t pins = 0;

    /* enable interrupt, mode timer, prescaler 16, trigger-auto, const follows */
    const uint8_t ctrl = Z80CTC_CTRL_EI|Z80CTC_CTRL_MODE_TIMER|
        Z80CTC_CTRL_PRESCALER_16|Z80CTC_CTRL_TRIGGER_AUTO|
        Z80CTC_CTRL_CONST_FOLLOWS|Z80CTC_CTRL_CONTROL;
    pins = _z80ctc_write(&ctc, pins, 1, ctrl);
    T(ctc.chn[1].control == ctrl);
    /* write timer constant */
    pins = _z80ctc_write(&ctc, pins, 1, 10);
    T(0 == (ctc.chn[1].control & Z80CTC_CTRL_CONST_FOLLOWS));
    T(10 == ctc.chn[1].constant);
    T(10 == ctc.chn[1].down_counter);
    for (int r = 0; r < 3; r++) {
        for (int i = 0; i < 160; i++) {
            pins = _z80ctc_tick_channel(&ctc, pins, 1);
            if (i != 159) {
                T(0 == (pins & Z80CTC_ZCTO1));
            }
        }
        T(pins & Z80CTC_ZCTO1);
        T(10 == ctc.chn[1].down_counter);
    }
}

void test_timer_wait_trigger() {
    z80ctc ctc;
    z80ctc_init(&ctc);
    uint64_t pins = 0;

    /* enable interrupt, mode timer, prescaler 16, trigger-wait, trigger-rising-edge, const follows */
    const uint8_t ctrl = Z80CTC_CTRL_EI|Z80CTC_CTRL_MODE_TIMER|
        Z80CTC_CTRL_PRESCALER_16|Z80CTC_CTRL_TRIGGER_WAIT|Z80CTC_CTRL_EDGE_RISING|
        Z80CTC_CTRL_CONST_FOLLOWS|Z80CTC_CTRL_CONTROL;
    pins = _z80ctc_write(&ctc, pins, 1, ctrl);
    T(ctc.chn[1].control == ctrl);
    /* write timer constant */
    pins = _z80ctc_write(&ctc, pins, 1, 10);
    T(0 == (ctc.chn[1].control & Z80CTC_CTRL_CONST_FOLLOWS));
    T(10 == ctc.chn[1].constant);

    /* tick the CTC without starting the timer */
    for (int i = 0; i < 300; i++) {
        pins = _z80ctc_tick_channel(&ctc, pins, 1);
        T(0 == (pins & Z80CTC_ZCTO1));
    }
    /* now start the timer on next tick */
    pins |= Z80CTC_CLKTRG1;
    for (int r = 0; r < 3; r++) {
        for (int i = 0; i < 160; i++) {
            pins = _z80ctc_tick_channel(&ctc, pins, 1);
            if (i != 159) {
                T(0 == (pins & Z80CTC_ZCTO1));
            }
            else {
                T(pins & Z80CTC_ZCTO1);
                T(10 == ctc.chn[1].down_counter);
            }
        }
    }
}

void test_counter() {
    z80ctc ctc;
    z80ctc_init(&ctc);
    uint64_t pins = 0;

    /* enable interrupt, mode counter, trigger-rising-edge, const follows */
    const uint8_t ctrl = Z80CTC_CTRL_EI|Z80CTC_CTRL_MODE_COUNTER|
        Z80CTC_CTRL_EDGE_RISING|Z80CTC_CTRL_CONST_FOLLOWS|Z80CTC_CTRL_CONTROL;
    pins = _z80ctc_write(&ctc, pins, 1, ctrl);
    T(ctc.chn[1].control == ctrl);
    /* write counter constant */
    pins = _z80ctc_write(&ctc, pins, 1, 10);
    T(0 == (ctc.chn[1].control & Z80CTC_CTRL_CONST_FOLLOWS));
    T(10 == ctc.chn[1].constant);

    /* trigger the CLKTRG1 pin */
    for (int r = 0; r < 3; r++) {
        for (int i = 0; i < 10; i++) {
            pins |= Z80CTC_CLKTRG1;
            pins = _z80ctc_tick_channel(&ctc, pins, 1);
            if (i != 9) {
                T(0 == (pins & Z80CTC_ZCTO1));
            }
            else {
                T(pins & Z80CTC_ZCTO1);
                T(10 == ctc.chn[1].down_counter);
            }
            pins &= ~Z80CTC_CLKTRG1;
            pins = _z80ctc_tick_channel(&ctc, pins, 1);
        }
    }
}

int main() {
    test_intvector();
    test_timer();
    test_timer_wait_trigger();
    test_counter();
    return 0;
}
