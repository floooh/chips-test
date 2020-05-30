//------------------------------------------------------------------------------
//  m6581-test.c
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/m6581.h"
#include "utest.h"

#define T(b) ASSERT_TRUE(b)

UTEST(m6581, rw) {
    (void)utest_result;
    m6581_t sid;
    m6581_init(&sid, &(m6581_desc_t){
        .tick_hz = 10000000,
        .sound_hz = 44100,
    });
    // FIXME
}


