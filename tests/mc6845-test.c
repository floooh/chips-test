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

// setup the MC6845 to a 80x24 characters at 60Hz configuration (see 'TABLE 7' in datasheet)
void setup_80x24(mc6845_t* crtc) {
    wr(crtc, MC6845_HTOTAL, 101);
    wr(crtc, MC6845_HDISPLAYED, 80);
    wr(crtc, MC6845_HSYNCPOS, 86);
    wr(crtc, MC6845_SYNCWIDTHS, 9);
    wr(crtc, MC6845_VTOTAL, 25);
    wr(crtc, MC6845_VTOTALADJ, 10);
    wr(crtc, MC6845_VDISPLAYED, 24);
    wr(crtc, MC6845_VSYNCPOS, 25);
    wr(crtc, MC6845_INTERLACEMODE, 0);
    wr(crtc, MC6845_MAXSCALINEADDR, 11);
    wr(crtc, MC6845_CURSORSTART, 0);
    wr(crtc, MC6845_CURSOREND, 11);
    wr(crtc, MC6845_STARTADDRHI, 0);
    wr(crtc, MC6845_STARTADDRLO, 128);
    wr(crtc, MC6845_CURSORHI, 0);
    wr(crtc, MC6845_CURSORLO, 128);
}

void test_write() {
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    
    // setup to a standard 80x24 config
    setup_80x24(&crtc);
    
    // ...and check that the values ended up in the right registers...
    T(crtc.h_total == 101);
    T(crtc.h_displayed == 80);
    T(crtc.h_sync_pos == 86);
    T(crtc.sync_widths == 9);
    T(crtc.v_total == 25);
    T(crtc.v_total_adjust == 10);
    T(crtc.v_displayed == 24);
    T(crtc.v_sync_pos == 25);
    T(crtc.interlace_mode == 0);
    T(crtc.max_scanline_addr == 11);
    T(crtc.cursor_start == 0);
    T(crtc.cursor_end == 11);
    T(crtc.start_addr_hi == 0);
    T(crtc.start_addr_lo == 128);
    T(crtc.cursor_hi == 0);
    T(crtc.cursor_lo == 128);
}

void test_hdisplayed() {
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    setup_80x24(&crtc);
    for (int tick = 0; tick < 1024; tick++) {
        mc6845_tick(&crtc);
        // need to run a complete line to 'warm up'
        if (crtc.scanline_ctr > 0) {
            // the h_de flag should be active from h_pos 0..79,
            // and inactive for the rest of the line
            if (crtc.h_ctr < 80) {
                T(crtc.h_de);
            }
            else {
                T(!crtc.h_de);
            }
        }
        else {
            // first scanline should have the de flag set yet(?)
            T(!crtc.h_de);
        }
    }
}

void test_hsync() {
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    setup_80x24(&crtc);
    for (int tick = 0; tick < 1024; tick++) {
        uint64_t pins = mc6845_tick(&crtc);
        if ((crtc.h_ctr >= 86) && (crtc.h_ctr <= 94)) {
            // inside hsync range
            T(crtc.hs);
            T(pins & MC6845_HS);
        }
        else {
            // before or after hsync range
            T(!crtc.hs);
            T(!(pins & MC6845_HS));
        }
    }
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

void test_hori_visual() {
    printf("\n\nMC6845 scanline visualization:\n\n");
    print_hori_ruler(4);
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    setup_80x24(&crtc);
    printf("\n%2d: ?", crtc.scanline_ctr);
    for (int tick = 0; tick < 1024; tick++) {
        mc6845_tick(&crtc);
        if (0 == crtc.h_ctr) {
            printf("\n%2d: ", crtc.scanline_ctr);
        }
        if (crtc.h_de) {
            putchar('D');
        }
        if (crtc.hs) {
            putchar('H');
        }
        if (!(crtc.h_de || crtc.hs)) {
            putchar('-');
        }
    }
    printf("...\n\n");
}

void test_scanline_row_visual() {
    printf("\n\nMC6845 scanline/row visualzation:\n\n");
    print_hori_ruler(7);
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    setup_80x24(&crtc);
    printf("\n%2d %2d: ?", crtc.scanline_ctr, crtc.row_ctr);
    for (int tick = 0; tick < 4096; tick++) {
        mc6845_tick(&crtc);
        if (0 == crtc.h_ctr) {
            printf("\n%2d %2d: ", crtc.scanline_ctr, crtc.row_ctr);
        }
        if (crtc.h_de) {
            putchar((crtc.row_ctr & 1) ? 'd':'D');
        }
        if (crtc.hs) {
            putchar('H');
        }
        if (!(crtc.h_de || crtc.hs)) {
            putchar('-');
        }
    }
    printf("...\n");
}

void test_frame_visual() {
    printf("\n\n\nMC6845 frame visualization:\n\n");
    print_hori_ruler(7);
    mc6845_t crtc;
    mc6845_init(&crtc, MC6845_TYPE_MC6845);
    setup_80x24(&crtc);
    bool valid = false;
    // horizontal DE flag won't be valid for the first scanline, and
    // the vertical DE flag for the first frame, instead of skipping the
    // first frame, we cheat here and set those flags manually
    crtc.h_de = crtc.v_de = true;
    for (int tick = 0; tick < 80000; tick++) {
        uint64_t pins = mc6845_tick(&crtc);
        if (crtc.scanline_ctr > 0) {
            valid = true;
        }
        if (valid) {
            if (0 == crtc.h_ctr) {
                printf("\n%2d %2d: ", crtc.scanline_ctr, crtc.row_ctr);
            }
            if (pins & MC6845_DE) {
                putchar('D');
            }
            if ((pins & (MC6845_HS|MC6845_VS)) == MC6845_HS) {
                putchar('H');
            }
            if ((pins & (MC6845_HS|MC6845_VS)) == MC6845_VS) {
                putchar('V');
            }
            if ((pins & (MC6845_HS|MC6845_VS)) == (MC6845_HS|MC6845_VS)) {
                putchar('X');
            }
            if (!(pins & (MC6845_DE|MC6845_HS|MC6845_VS))) {
                if (crtc.scanline_adjust) {
                    putchar('.');
                }
                else {
                    putchar('-');
                }
            }
        }
    }
    printf("...\n");
}

int main() {
    test_write();
    test_hdisplayed();
    test_hsync();
    test_hori_visual();
    test_scanline_row_visual();
    test_frame_visual();
}

