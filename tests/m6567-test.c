//------------------------------------------------------------------------------
//  m6567-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6567.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

uint32_t rgba8_buffer[262144];

uint16_t fetch(uint16_t addr) {
    // FIXME
    return 0;
}

void test_rw() {
    m6567_t vic;
    m6567_init(&vic, &(m6567_desc_t){
        .type = M6567_TYPE_6569,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = fetch
    });
    int w, h;
    m6567_display_size(&vic, &w, &h);
    T(w == 403);
    T(h == 284);
}

int main() {
    test_rw();
    return 0;
}
