//------------------------------------------------------------------------------
//  m6581-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6581.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

void test_rw() {
    m6581_t sid;
    m6581_init(&sid, &(m6581_desc_t){
        .tick_hz = 10000000,
        .sound_hz = 44100,
    });
    // FIXME
}

int main() {
    test_rw();
    return 0;
}


