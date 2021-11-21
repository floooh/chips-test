//------------------------------------------------------------------------------
//  z80ctc_test.c
//------------------------------------------------------------------------------
// force assert() enabled
#include "chips/z80.h"
#define CHIPS_IMPL
#include "chips/z80ctc.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(z80ctc, intvector) {
    z80ctc_t ctc;
    z80ctc_init(&ctc);
    /* write interrupt vector */
    _z80ctc_write(&ctc, 0, 0, 0xE0);
    T(ctc.chn[0].int_vector == 0xE0);
    T(ctc.chn[1].int_vector == 0xE2);
    T(ctc.chn[2].int_vector == 0xE4);
    T(ctc.chn[3].int_vector == 0xE6);
}

UTEST(z80ctc, timer) {
    z80ctc_t ctc;
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
            pins = z80ctc_tick(&ctc, pins);
            if (i != 159) {
                T(0 == (pins & Z80CTC_ZCTO1));
            }
        }
        T(pins & Z80CTC_ZCTO1);
        T(10 == ctc.chn[1].down_counter);
    }
}

UTEST(z80ctc, timer_wait_trigger) {
    z80ctc_t ctc;
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
        pins = z80ctc_tick(&ctc, pins);
        T(0 == (pins & Z80CTC_ZCTO1));
    }
    /* now start the timer on next tick */
    pins |= Z80CTC_CLKTRG1;
    pins = z80ctc_tick(&ctc, pins);
    for (int r = 0; r < 3; r++) {
        for (int i = 0; i < 160; i++) {
            pins = z80ctc_tick(&ctc, pins);
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

UTEST(z80ctc, counter) {
    z80ctc_t ctc;
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
            pins = z80ctc_tick(&ctc, pins);
            if (i != 9) {
                T(0 == (pins & Z80CTC_ZCTO1));
            }
            else {
                T(pins & Z80CTC_ZCTO1);
                T(10 == ctc.chn[1].down_counter);
            }
            pins &= ~Z80CTC_CLKTRG1;
            pins = z80ctc_tick(&ctc, pins);
        }
    }
}

/* a complete, integrated interrupt handling test */
static z80_t cpu;
static z80ctc_t ctc;
static uint8_t mem[1<<16];

static uint64_t tick(uint64_t pins) {
    pins = z80_tick(&cpu, pins);
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
        }
        else if (pins & Z80_WR) {
            mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
        }
    }
    else if (pins & Z80_IORQ) {
        /* just assume that any IO request is for the CTC */
        pins = (pins & Z80_PIN_MASK) | Z80CTC_CE;
        if (pins & 1) pins |= Z80CTC_CS0;
        if (pins & 2) pins |= Z80CTC_CS1;
    }
    pins = z80ctc_tick(&ctc, pins | Z80_IEIO);
    return pins & Z80_PIN_MASK;
}

static void w16(uint16_t addr, uint16_t data) {
    mem[addr]   = (uint8_t) data;
    mem[addr+1] = (uint8_t) (data>>8);
}

static void copy(uint16_t addr, uint8_t* bytes, size_t num) {
    assert((addr + num) <= sizeof(mem));
    memcpy(&mem[addr], bytes, num);
}

UTEST(z80ctc, interrupt) {
    z80_init(&cpu);
    z80ctc_init(&ctc);
    memset(mem, 0, sizeof(mem));

    /*
        - setup CTC channel 0 to request an interrupt every 1024 ticks
        - go into a halt, which is left at interrupt, increment a
          memory location, and loop back to the halt
        - an interrupt routine increments another memory location
        - run CPU for N ticks, check if both counters have expected values
    */

    /* location of interrupt routine */
    w16(0x00E0, 0x0200);

    uint8_t ctc_ctrl =
        Z80CTC_CTRL_EI |
        Z80CTC_CTRL_MODE_TIMER |
        Z80CTC_CTRL_PRESCALER_256 |
        Z80CTC_CTRL_TRIGGER_AUTO |
        Z80CTC_CTRL_CONST_FOLLOWS |
        Z80CTC_CTRL_CONTROL;

    /* main program at 0x0100 */
    uint8_t main_prog[] = {
        0x31, 0x00, 0x03,   /* LD SP,0x0300 */
        0xFB,               /* EI */
        0xED, 0x5E,         /* IM 2 */
        0xAF,               /* XOR A */
        0xED, 0x47,         /* LD I,A */
        0x3E, 0xE0,         /* LD A,0xE0: load interrupt vector into CTC channel 0 */
        0xD3, 0x00,         /* OUT (0),A */
        0x3E, ctc_ctrl,     /* LD A,n: configure CTC channel 0 as timer */
        0xD3, 0x00,         /* OUT (0),A */
        0x3E, 0x04,         /* LD A,0x04: timer constant (with prescaler 256): 4 * 256 = 1024 */
        0xD3, 0x00,         /* OUT (0),A */
        0x76,               /* HALT */
        0x21, 0x00, 0x00,   /* LD HL,0x0000 */
        0x34,               /* INC (HL) */
        0x18, 0xF9,         /* JR -> HALT, endless loop back to the HALT instruction */
    };
    copy(0x0100, main_prog, sizeof(main_prog));

    /* interrupt service routine, increment content of 0x0001 */
    uint8_t int_prog[] = {
        0xF3,               /* DI */
        0x21, 0x01, 0x00,   /* LD HL,0x0001 */
        0x34,               /* INC (HL) */
        0xFB,               /* EI */
        0xED, 0x4D,         /* RETI */
    };
    copy(0x0200, int_prog, sizeof(int_prog));

    /* run for 4500 ticks, this should invoke the interrupt routine 4x */
    uint64_t pins = z80_prefetch(&cpu, 0x0100);
    for (int i = 0; i < 4500; i++) {
        pins = tick(pins);
    }
    T(mem[0x0000] == 4);
    T(mem[0x0001] == 4);
}
