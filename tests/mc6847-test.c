//------------------------------------------------------------------------------
//  mc6847-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/mc6847.h"
#include "utest.h"

static uint32_t rgba8_buffer[MC6847_DISPLAY_WIDTH * MC6847_DISPLAY_HEIGHT];

static uint64_t fetch(uint64_t pins, void* user_data) {
    return pins;
}

UTEST(mc6847, setup) {
    mc6847_t vdg;
    mc6847_desc_t desc = {
        .tick_hz = 1000000,
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = fetch,
        .user_data = 0
    };
    mc6847_init(&vdg, &desc);
}
