//------------------------------------------------------------------------------
//  c64-sixel.c
//
//  A C64 emulator for the terminal, using Sixel graphics for the video 
//  output:
//
//  https://en.wikipedia.org/wiki/Sixel
//
//  Tested with iTerm2 3.x+ on macOS.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <curses.h>     // curses is only used for non-blocking keyboard input
#include <unistd.h>
#include <signal.h>
#define SOKOL_IMPL
#include "sokol_time.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#include "c64-roms.h"

static struct {
    c64_t c64;
    uint32_t pixels[256*1024];  // RGBA8 buffer for emulator's video output
    char chrs[256*1024];        // the video output converted to Sixel ASCII characters
} state;

#define FRAME_USEC (33333)

// a signal handler for Ctrl-C, for proper cleanup 
static int quit_requested = 0;
static void catch_sigint(int signo) {
    (void)signo;
    quit_requested = 1;
}

// move cursor back to topleft
static void term_home(void) {
    printf("\033[0;0H");
}

// Esc sequence to switch terminal into Sixel mode
static void term_sixel_begin(void) {
    const int w = 2 * c64_display_width(&state.c64);
    const int h = 2 * c64_display_height(&state.c64);
    printf("\033Pq\"1;1;%d;%d", w, h);
}

// TODO: define the Sixel color palette
static void term_sixel_colors(void) {
    printf("#0;2;0;0;0"         // black
           "#1;2;100;100;100");   // white
}

// flush terminal output for current frame
static void term_flush(void) {
    printf("\e\\\n");
}

// decode the pixel buffer into a sixel character sequence and send to terminal
static void term_sixel_pixels(void) {
    const int w = c64_display_width(&state.c64);
    const int h = c64_display_height(&state.c64);
    int chr_pos = 0;
    for (int y = 0; y < h; y += 3) {
        for (int x = 0; x < w; x++) {
            char chr = 0;
            for (int i = 0; i < 3; i++) {
                uint32_t p = state.pixels[(y+i)*w + x];
                // FIXME: convert pixel color back to terminal color
                if (p == _m6569_colors[6]) {
                    chr |= (3<<(i*2));
                }
            }
            state.chrs[chr_pos++] = chr + 0x3F;
            state.chrs[chr_pos++] = chr + 0x3F;
        }
        state.chrs[chr_pos++] = '$';
        state.chrs[chr_pos++] = '-';
    }
    state.chrs[chr_pos++] = 0;
    printf("%s", state.chrs);
}

int main() {    
    
    // install a Ctrl-C signal handler
    signal(SIGINT, catch_sigint);

    // setup curses for non-blocking IO
    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    // sokol time for rendering frames at correct speed
    stm_setup();

    // setup the C64 emulator
    c64_init(&state.c64, &(c64_desc_t){
        .pixel_buffer = state.pixels,
        .pixel_buffer_size = sizeof(state.pixels),
        .rom_char = dump_c64_char_bin,
        .rom_char_size = sizeof(dump_c64_char_bin),
        .rom_basic = dump_c64_basic_bin,
        .rom_basic_size = sizeof(dump_c64_basic_bin),
        .rom_kernal = dump_c64_kernalv3_bin,
        .rom_kernal_size = sizeof(dump_c64_kernalv3_bin)
    });

    while (!quit_requested) {
        uint64_t start = stm_now();

        // run emulation for one frame
        c64_exec(&state.c64, FRAME_USEC);
        
        // keyboard input
        int ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case 10:  ch = 0x0D; break; // ENTER
                case 127: ch = 0x01; break; // BACKSPACE
                case 27:  ch = 0x03; break; // ESCAPE
                case 260: ch = 0x08; break; // LEFT
                case 261: ch = 0x09; break; // RIGHT
                case 259: ch = 0x0B; break; // UP
                case 258: ch = 0x0A; break; // DOWN
            }
            if (ch > 32) {
                if (islower(ch)) {
                    ch = toupper(ch);
                }
                else if (isupper(ch)) {
                    ch = tolower(ch);
                }
            }
            if (ch < 256) {
                c64_key_down(&state.c64, ch);
                c64_key_up(&state.c64, ch);
            }
        }

        // render the frame in the terminal
        term_home();
        term_sixel_begin();
        term_sixel_colors();
        term_sixel_pixels();
        term_flush();

        // sleep until next frame
        double dur_usec = stm_us(stm_since(start));
        if (dur_usec < FRAME_USEC) {
            usleep(FRAME_USEC - dur_usec);
        }
    }
    endwin();
    return 0;
}
