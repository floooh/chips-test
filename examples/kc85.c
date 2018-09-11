/*
    kc85.c

    KC85/4 emulator in a terminal. Requires a UNIX environment.
*/
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "roms/kc85-roms.h"
#include <curses.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

kc85_t kc85;

// run the emulator and render-loop at 30fps
#define FRAME_USEC (33333)

// a signal handler for Ctrl-C, for proper cleanup 
static int quit_requested = 0;
static void catch_sigint(int signo) {
    quit_requested = 1;
}

void init_kc_colors(void) {
    start_color();

    // KC85/4 background colors (darker than foreground)
    init_color(0, 0, 0, 0);            // black
    init_color(1, 0, 0, 640);          // dark-blue
    init_color(2, 640, 0, 0);          // dark-red
    init_color(3, 640, 0, 640);        // dark-magenta
    init_color(4, 0, 640, 0);          // dark-green
    init_color(5, 0, 640, 640);        // dark-cyan
    init_color(6, 640, 640, 0);        // dark-yellow
    init_color(7, 640, 640, 640);      // grey

    // KC85/4 foreground colors
    init_color(8, 0, 0, 0);             // black
    init_color(9, 0, 0, 1000);          // blue
    init_color(10, 1000, 0, 0);         // red
    init_color(11, 1000, 0, 1000);      // magenta
    init_color(12, 0, 1000, 0);         // green
    init_color(13, 0, 1000, 1000);      // cyan
    init_color(14, 1000, 1000, 0);      // yellow
    init_color(15, 1000, 1000, 1000);   // white
    init_color(16, 0, 0, 0);            // black #2
    init_color(17, 1000, 0, 640);       // violet
    init_color(18, 1000, 640, 0);       // orange
    init_color(19, 1000, 0, 640);       // purple
    init_color(20, 0, 1000, 640);       // blueish-green
    init_color(21, 0, 640, 1000);       // greenish-blue
    init_color(22, 640, 1000, 0);       // yellow-green
    init_color(23, 1000, 1000, 1000);   // white

    // setup 128 color pairs with all color combinations
    for (int fg = 0; fg < 16; fg++) {
        for (int bg = 0; bg < 8; bg++) {
            int cp = (fg*8 + bg) + 1;
            init_pair(cp, fg+8, bg);
        }
    }
}

int main() {
    /* initialize a KC85/4 emulator instance, we don't need audio 
       or video output, so don't provide a pixel buffer and
       audio callback
    */
    kc85_init(&kc85, &(kc85_desc_t){
        .type = KC85_TYPE_4,
        .rom_caos42c = dump_caos42c,
        .rom_caos42c_size = sizeof(dump_caos42c),
        .rom_caos42e = dump_caos42e,
        .rom_caos42e_size = sizeof(dump_caos42e),
        .rom_kcbasic = dump_basic_c0,
        .rom_kcbasic_size = sizeof(dump_basic_c0)
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

        // render the display 
        int cursor_x = kc85.ram[4][0x37A0];
        int cursor_y = kc85.ram[4][0x37A1];
        int cur_color_pair = -1;
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 40; x++) {
                // get color code from color buffer, note the color buffer is 90 degree rotated!
                int color_pair = (kc85.ram[5][x*256 + y*8] & 0x7f) + 1;
                if (color_pair != cur_color_pair) {
                    attron(COLOR_PAIR(color_pair));
                }
                // get character code from ASCII buffer
                char chr = (char) kc85.ram[4][0x3200 + y*40 + x];
                if ((chr < 20) || (chr > 127)) {
                    chr = 0x20;
                }
                // padding to get proper aspect ratio
                mvaddch(y, x*2, ' ');
                // cursor on?
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

        // pause until for next frame
        usleep(FRAME_USEC);
    }
    endwin();
    return 0;
}