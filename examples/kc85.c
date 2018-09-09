/*
    kc85.c

    KC85/2, /3 and /4.
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
#include <ncurses.h>
#include <unistd.h>
#include <ctype.h>

kc85_t kc85;
uint32_t fb[KC85_DISPLAY_WIDTH * KC85_DISPLAY_HEIGHT];

/* refresh at 30fps */
#define FRAME_USEC (33333)

/* convert a KC color to ncurses color. KC85 has 16 foreground colors, ncurses
only 8, so we duplicate the foreground colors
*/
int kc85_to_curses_color(int c) {
    switch (c & 7) {
        case 0: return COLOR_BLACK;
        case 1: return COLOR_BLUE;
        case 2: return COLOR_RED;
        case 3: return COLOR_MAGENTA;
        case 4: return COLOR_GREEN;
        case 5: return COLOR_CYAN;
        case 6: return COLOR_YELLOW;
        default: return COLOR_WHITE;
    }
}

int main() {
    kc85_init(&kc85, &(kc85_desc_t){
        .type = KC85_TYPE_4,
        .pixel_buffer = fb,
        .pixel_buffer_size = sizeof(fb),
        .rom_caos42c = dump_caos42c,
        .rom_caos42c_size = sizeof(dump_caos42c),
        .rom_caos42e = dump_caos42e,
        .rom_caos42e_size = sizeof(dump_caos42e),
        .rom_kcbasic = dump_basic_c0,
        .rom_kcbasic_size = sizeof(dump_basic_c0)
    });

    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    /* setup color pairs, KC85 has 16 foreground and 8 background colors, 
       ncurses has only 8 color slots
    */
    start_color();
    init_color(COLOR_BLACK, 0, 0, 0);
    init_color(COLOR_BLUE, 0, 0, 1000);
    init_color(COLOR_RED, 1000, 0, 0);
    init_color(COLOR_MAGENTA, 1000, 0, 1000);
    init_color(COLOR_GREEN, 0, 1000, 0);
    init_color(COLOR_CYAN, 0, 1000, 1000);
    init_color(COLOR_YELLOW, 1000, 1000, 0);
    init_color(COLOR_WHITE, 1000, 1000, 1000);
    for (int fg = 0; fg < 16; fg++) {
        for (int bg = 0; bg < 8; bg++) {
            int curses_fg = kc85_to_curses_color(fg);
            int curses_bg = kc85_to_curses_color(bg);
            int index = fg*8 + bg;
            assert(index < COLOR_PAIRS);
            init_pair(index, curses_fg, curses_bg);
        }
    }
    while (true) {
        /* tick the emulator */
        kc85_exec(&kc85, FRAME_USEC);

        /* keyboard input */
        int ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case 10:  ch = 0x0D; break; /* ENTER */
                case 127: ch = 0x01; break; /* BACKSPACE */
                case 27:  ch = 0x03; break; /* ESCAPE */
                case 260: ch = 0x08; break; /* LEFT */
                case 261: ch = 0x09; break; /* RIGHT */
                case 259: ch = 0x0B; break; /* UP */
                case 258: ch = 0x0A; break; /* DOWN */
                default: break;
            }
            if (islower(ch)) {
                ch = toupper(ch);
            }
            else if (isupper(ch)) {
                ch = tolower(ch);
            }
            if (ch < 256) {
                kc85_key_down(&kc85, ch);
                kc85_key_up(&kc85, ch);
            }
        }

        /* render the display */
        int cursor_x = kc85.ram[4][0x37A0];
        int cursor_y = kc85.ram[4][0x37A1];
        int cur_color_byte = -1;
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 40; x++) {
                /* get color code from color buffer, note the color buffer is 90 degree rotated! */
                int color_byte = kc85.ram[5][x*256 + y*8] & 0x7f;
                if (color_byte != cur_color_byte) {
                    attron(COLOR_PAIR(color_byte));
                }
                /* padding to get 4x3 aspect ratio */
                mvaddch(y, x*2, ' ');
                /* get character code from ASCII buffer */
                char chr = (char) kc85.ram[4][0x3200 + y*40 + x];
                if ((chr < 20) || (chr > 127)) {
                    chr = 0x20;
                }
                /* cursor on? */
                if ((x == cursor_x) && (y == cursor_y)) {
                    attrset(A_REVERSE);
                }
                /* render character */
                mvaddch(y, x*2+1, chr);
                /* cursor off? */
                if ((x == cursor_x) && (y == cursor_y)) {
                    attrset(A_NORMAL);
                }
            }
        }
        refresh();

        /* wait for next frame */
        usleep(FRAME_USEC);
    }
    endwin();
    return 0;
}