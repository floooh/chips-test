//------------------------------------------------------------------------------
//  mc6845-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/mc6845.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

int main() {
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);

    /* write the HDISPLAYED register */
    uint64_t pins = MC6845_CS|MC6845_RS|MC6845_RW;
    MC6845_SET_DATA(pins, MC6845_REG_HDISPLAYED);
    mc6845_iorq(&crtc, pins);
    T(crtc.sel == MC6845_REG_HDISPLAYED);
    pins = MC6845_CS|MC6845_RW;
    MC6845_SET_DATA(pins, 0x23);
    mc6845_iorq(&crtc, pins);
    T(0x23 == crtc.reg[MC6845_REG_HDISPLAYED]);
}

