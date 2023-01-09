//------------------------------------------------------------------------------
//  mc6847-test.c
//------------------------------------------------------------------------------
#include "chips/chips_common.h"
#define CHIPS_IMPL
#include "chips/mc6847.h"
#include "utest.h"

static uint8_t framebuffer[MC6847_FRAMEBUFFER_SIZE_BYTES];

static uint64_t fetch(uint64_t pins, void* user_data) {
    (void)user_data;
    return pins;
}

UTEST(mc6847, setup) {
    (void)utest_result;
    mc6847_t vdg;
    mc6847_desc_t desc = {
        .tick_hz = 1000000,
        .framebuffer = {
            .ptr = framebuffer,
            .size = sizeof(framebuffer)
        },
        .fetch_cb = fetch,
        .user_data = 0
    };
    mc6847_init(&vdg, &desc);
}
