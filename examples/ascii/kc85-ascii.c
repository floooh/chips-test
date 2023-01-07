/*
    kc85.c

    Stripped down KC85/4 emulator running in a (xterm-256color) terminal,
    rendering the ASCII buffer of the KC85 through curses. Requires a UNIX
    environment to build and run (tested on OSX and Linux).
*/
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include "keybuf.h"
#define SOKOL_ARGS_IMPL
#include "sokol_args.h"
#define CHIPS_IMPL
#define CHIPS_KC85_TYPE_4
#include "chips/chips_common.h"
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "kc85-roms.h"

static kc85_t kc85;

/* frame counter for delayed actions */
static uint32_t frame_count;

// run the emulator and render-loop at 30fps
#define FRAME_USEC (33333)

// a signal handler for Ctrl-C, for proper cleanup
static int quit_requested = 0;
static void catch_sigint(int signo) {
    (void)signo;
    quit_requested = 1;
}

// xterm-color256 color codes mapped to KC85 colors, don't use the colors
// below 16 as those a most likely mapped by color themes
static int background_colors[8] = {
    16,     // black
    19,     // dark-blue
    124,    // dark-red
    127,    // dark-magenta
    34,     // dark-green
    37,     // dark-cyan
    142,    // dark-yellow
    145,    // dark-grey
};

static int foreground_colors[16] = {
    16,     // black
    21,     // blue
    196,    // red
    201,    // magenta
    46,     // green
    51,     // cyan
    226,    // yellow
    231,    // white
    16,     // black #2
    199,    // violet
    214,    // orange
    129,    // purple
    49,     // blue-green
    39,     // green-blue
    154,    // yellow-green
    231,     // white #2
};

static void init_kc_colors(void) {
    start_color();
    // setup 128 color pairs with all color combinations, using
    // the standard xterm-256color palette
    for (int fg = 0; fg < 16; fg++) {
        for (int bg = 0; bg < 8; bg++) {
            int cp = (fg*8 + bg) + 1;
            init_pair(cp, foreground_colors[fg], background_colors[bg]);
        }
    }
}

int main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames = 10 });

    // initialize a KC85/4 emulator instance, we don't need audio
    // or video output, so don't provide a pixel buffer and
    // audio callback
    kc85_init(&kc85, &(kc85_desc_t){
        .roms = {
            .caos42c = { .ptr=dump_caos42c_854, .size=sizeof(dump_caos42c_854) },
            .caos42e = { .ptr=dump_caos42e_854, .size=sizeof(dump_caos42e_854) },
            .kcbasic = { .ptr=dump_basic_c0_853, .size=sizeof(dump_basic_c0_853) }
        }
    });

    // install a Ctrl-C signal handler
    signal(SIGINT, catch_sigint);

    // setup curses
    initscr();
    init_kc_colors();
    noecho();
    curs_set(FALSE);
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    attron(A_BOLD);

    // run the emulation/input/render loop
    while (!quit_requested) {
        frame_count++;

        // tick the emulator for 1 frame
        kc85_exec(&kc85, FRAME_USEC);

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
                default: break;
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
                kc85_key_down(&kc85, ch);
                kc85_key_up(&kc85, ch);
            }
        }
        uint8_t key_code;
        if (0 != (key_code = keybuf_get(FRAME_USEC))) {
            kc85_key_down(&kc85, key_code);
            kc85_key_up(&kc85, key_code);
        }

        // render the display, most of this is an alternative video-memory
        // decoding, instead of the standard decoding to RGBA8 pixels, we'll decode
        // to curses color pairs and ASCII codes
        uint32_t cursor_x = kc85.ram[4][0x37A0];
        uint32_t cursor_y = kc85.ram[4][0x37A1];
        int cur_color_pair = -1;
        for (uint32_t y = 0; y < 32; y++) {
            for (uint32_t x = 0; x < 40; x++) {
                // get color code from color buffer, different location and
                // layout on KC85/2,/3 vs KC85/4!
                uint8_t color_byte;
                // video memory on KC85/4 is 90 degree rotated and
                // there are 2 color memory banks
                int irm_bank = (kc85.io84 & 1) * 2;
                color_byte = kc85.ram[5+irm_bank][x*256 + y*8];
                int color_pair = ((int)(color_byte & 0x7F))+1;
                if (color_pair != cur_color_pair) {
                    attron(COLOR_PAIR(color_pair));
                    cur_color_pair = color_pair;
                }

                // get ASCII code from character buffer at 0xB200
                char chr = (char) kc85.ram[4][0x3200 + y*40 + x];
                // get character code from ASCII buffer
                if ((chr < 32) || (chr > 126)) {
                    chr = 32;
                }
                // padding to get proper aspect ratio
                mvaddch(y, x*2, ' ');
                // on KC85_4 we need to render an ASCII cursor, the 85/2 and /3
                // implement the cursor through a color byte
                if ((x == cursor_x) && (y == cursor_y)) {
                    attron(A_UNDERLINE);
                }
                // character
                mvaddch(y, x*2+1, chr);
                // cursor off?
                if ((x == cursor_x) && (y == cursor_y)) {
                    attroff(A_UNDERLINE);
                }
            }
        }
        refresh();

        // pause until next frame
        usleep(FRAME_USEC);
    }
    endwin();
    return 0;
}
