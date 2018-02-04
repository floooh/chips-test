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

void test_write() {
    m6522_t via;
    m6522_init(&via, in_cb, out_cb);


}

void test_read() {

}

int main() {
    test_write();
    test_read();
    return 0;
}