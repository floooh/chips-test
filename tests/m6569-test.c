//------------------------------------------------------------------------------
//  m6569-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/m6569.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

static uint32_t rgba8_buffer[262144];

static uint16_t fetch(uint16_t addr, void* user_data) {
    // FIXME
    (void)addr;
    (void)user_data;
    return 0;
}

UTEST(m6569, rw) {
    m6569_t vic;
    m6569_init(&vic, &(m6569_desc_t){
        .rgba8_buffer = rgba8_buffer,
        .rgba8_buffer_size = sizeof(rgba8_buffer),
        .fetch_cb = fetch,
        .user_data = 0,
        .vis_x = 64,
        .vis_y = 24,
        .vis_w = 392,
        .vis_h = 272,
    });
    T(m6569_display_width(&vic) == 392);
    T(m6569_display_height(&vic) == 272);
}
