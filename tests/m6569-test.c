//------------------------------------------------------------------------------
//  m6569-test.c
//------------------------------------------------------------------------------
#include "chips/chips_common.h"
#define CHIPS_IMPL
#include "chips/m6569.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

static uint8_t framebuffer[M6569_FRAMEBUFFER_SIZE_BYTES];

static uint16_t fetch(uint16_t addr, void* user_data) {
    // FIXME
    (void)addr;
    (void)user_data;
    return 0;
}

UTEST(m6569, rw) {
    m6569_t vic;
    m6569_init(&vic, &(m6569_desc_t){
        .framebuffer = {
            .ptr = framebuffer,
            .size = sizeof(framebuffer)
        },
        .fetch_cb = fetch,
        .user_data = 0,
        .screen = {
            .x = 64,
            .y = 24,
            .width = 392,
            .height = 272,
        }
    });
    T(m6569_screen(&vic).width == 392);
    T(m6569_screen(&vic).height == 272);
}
