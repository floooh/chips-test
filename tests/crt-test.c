//------------------------------------------------------------------------------
//  crt-test.c
//------------------------------------------------------------------------------
// force assert() enabled
#ifdef NDEBUG
#undef NDEBUG
#endif
#define CHIPS_IMPL
#include "chips/crt.h"
#include "chips/mc6845.h"
#include <stdio.h>

uint32_t num_tests = 0;
#define T(x) { assert(x); num_tests++; }

// write value to the MC6845
void wr(mc6845_t* crtc, uint8_t reg, uint8_t val) {
    uint64_t pins = MC6845_CS|MC6845_RS;
    // first write register address
    MC6845_SET_DATA(pins, reg);
    mc6845_iorq(crtc, pins);
    // next write the value
    pins = MC6845_CS;
    MC6845_SET_DATA(pins, val);
    mc6845_iorq(crtc, pins);
}

void print_hori_ruler(int start) {
    for (int i = 0; i < start; i++) {
        putchar(' ');
    }
    for (int i = 0; i < 120; i++) {
        printf("%d", i % 10);
    }
    putchar('\n');
    for (int i = 0; i < start; i++) {
        putchar(' ');
    }
    for (int i = 0; i < 120; i++) {
        if ((i % 10) == 0) {
            printf("%d", i/10);
        }
        else {
            putchar(' ');
        }
    }
    putchar('\n');
}

int main() {
    // setup a MC6845 to provide the HSYNC and VSYNC signals for the CRT,
    // and print the CRT state to the console

    // setup the MC6845 to the CPC's standard 300x200 (40x25) resolution
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    wr(&crtc, MC6845_HTOTAL, 0x3F);
    wr(&crtc, MC6845_HDISPLAYED, 0x28);
    wr(&crtc, MC6845_HSYNCPOS, 0x2E);
    wr(&crtc, MC6845_SYNCWIDTHS, 0x8E);
    wr(&crtc, MC6845_VTOTAL, 0x26);
    wr(&crtc, MC6845_VTOTALADJ, 0x00);
    wr(&crtc, MC6845_VDISPLAYED, 0x19);
    wr(&crtc, MC6845_VSYNCPOS, 0x1E);
    wr(&crtc, MC6845_INTERLACEMODE, 0x00);
    wr(&crtc, MC6845_MAXSCALINEADDR, 0x07);

    // setup the CRT
    crt_t crt;
    crt_init(&crt, CRT_PAL, 2, 32, 48, 272);

    print_hori_ruler(6);

    // ...and tick both systems
    for (int i = 0; i < 20000; i++) {

        if (crt.h_pos == 0) {
            printf("\n %03d: ", crt.v_pos);
        }
        if (crt.v_blank) {
            putchar('v');
        }
        else if (crt.h_blank) {
            putchar('h');
        }
        else {
            putchar('.');
        }
        uint64_t pins = mc6845_tick(&crtc);
        bool hsync = pins & MC6845_HS;
        bool vsync = pins & MC6845_VS;
        crt_tick(&crt, hsync, vsync);
    }
}
