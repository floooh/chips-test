//------------------------------------------------------------------------------
//  m6522-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6522.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

uint8_t values[M6522_NUM_PORTS] = { 0 };

void out_cb(int port_id, uint8_t data) {
    values[port_id] = data;
}

uint8_t in_cb(int port_id) {
    return values[port_id];
}

void test_rw() {
    m6522_t via;
    m6522_init(&via, in_cb, out_cb);

    /* write DDR and OUT value of port B */
    uint64_t pins = M6522_CS1;
    M6522_SET_ADDR(pins, M6522_REG_DDRB);
    M6522_SET_DATA(pins, 0xAA);
    pins = m6522_iorq(&via, pins);
    T(via.ddr_b == 0xAA);
    T(values[M6522_PORT_B] == 0x55);
    M6522_SET_ADDR(pins, M6522_REG_RB);
    M6522_SET_DATA(pins, 0xF0);
    pins = m6522_iorq(&via, pins);
    T(via.out_b = 0xF0);
    T(values[M6522_PORT_B] == 0xF5);

    /* read back B values */
    pins = M6522_CS1|M6522_RW;
    M6522_SET_ADDR(pins, M6522_REG_DDRB);
    pins = m6522_iorq(&via, pins);
    T(M6522_GET_DATA(pins) == 0xAA);
    M6522_SET_ADDR(pins, M6522_REG_RB);
    pins = m6522_iorq(&via, pins);
    T(M6522_GET_DATA(pins) == 0xF5);
}

int main() {
    test_rw();
    return 0;
}