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

// write value to the MC6845
void wr(mc6845_t* crtc, uint8_t reg, uint8_t val) {
    uint64_t pins = MC6845_CS|MC6845_RS|MC6845_RW;
    // first write register address
    MC6845_SET_DATA(pins, reg);
    mc6845_iorq(crtc, pins);
    // next write the value
    pins = MC6845_CS|MC6845_RW;
    MC6845_SET_DATA(pins, val);
    mc6845_iorq(crtc, pins);
}

void test_write() {
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);

    // setup the MC6845 to a 80x24 configuration (see 'TABLE 7' in datasheet)
    wr(&crtc, MC6845_HTOTAL, 101);
    wr(&crtc, MC6845_HDISPLAYED, 80);
    wr(&crtc, MC6845_HSYNCPOS, 86);
    wr(&crtc, MC6845_SYNCWIDTHS, 9);
    wr(&crtc, MC6845_VTOTAL, 24);
    wr(&crtc, MC6845_VTOTALADJ, 10);
    wr(&crtc, MC6845_VDISPLAYED, 24);
    wr(&crtc, MC6845_VSYNCPOS, 24);
    wr(&crtc, MC6845_INTERLACEMODE, 0);
    wr(&crtc, MC6845_MAXSCALINEADDR, 11);
    wr(&crtc, MC6845_CURSORSTART, 0);
    wr(&crtc, MC6845_CURSOREND, 11);
    wr(&crtc, MC6845_STARTADDRHI, 0);
    wr(&crtc, MC6845_STARTADDRLO, 128);
    wr(&crtc, MC6845_CURSORHI, 0);
    wr(&crtc, MC6845_CURSORLO, 128);
    
    // ...and check that the values ended up in the right registers...
    T(crtc.h_total == 101);
    T(crtc.h_displayed == 80);
    T(crtc.h_sync_pos == 86);
    T(crtc.sync_widths == 9);
    T(crtc.v_total == 24);
    T(crtc.v_total_adjust == 10);
    T(crtc.v_displayed == 24);
    T(crtc.v_sync_pos == 24);
    T(crtc.interlace_mode == 0);
    T(crtc.max_scanline_addr == 11);
    T(crtc.cursor_start == 0);
    T(crtc.cursor_end == 11);
    T(crtc.start_addr_hi == 0);
    T(crtc.start_addr_lo == 128);
    T(crtc.cursor_hi == 0);
    T(crtc.cursor_lo == 128);
}

int main() {
    test_write();
}

