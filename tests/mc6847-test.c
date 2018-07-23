//------------------------------------------------------------------------------
//  mc6847-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/mc6847.h"
#include <stdio.h>

uint32_t rgba8_buffer[MC6847_DISPLAY_WIDTH * MC6847_DISPLAY_HEIGHT];

uint64_t fetch(uint64_t pins, void* user_data) {
    return pins;
}

int main() {
    mc6847_t vdg;
    mc6847_desc_t desc = {
        .tick_hz = 1000000,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = fetch,
        .user_data = 0
    };
    mc6847_init(&vdg, &desc);

    return 0;
}
