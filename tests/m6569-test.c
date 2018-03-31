//------------------------------------------------------------------------------
//  m6569-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/m6569.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

uint32_t rgba8_buffer[262144];

uint16_t fetch(uint16_t addr) {
    // FIXME
    return 0;
}

void test_rw() {
    m6569_t vic;
    m6569_init(&vic, &(m6569_desc_t){
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = fetch,
        .vis_x = 64,
        .vis_y = 24,
        .vis_w = 392,
        .vis_h = 272,
    });
    int w, h;
    m6569_display_size(&vic, &w, &h);
    T(w == 392);
    T(h == 272);
}

int main() {
    test_rw();
    return 0;
}
