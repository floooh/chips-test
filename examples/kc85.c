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

int main() {
    kc85_init(&kc85, &(kc85_desc_t){
        .type = KC85_TYPE_4,
        .pixel_buffer = fb,
        .pixel_buffer_size = sizeof(fb),
        .rom_caos22 = dump_caos22,
        .rom_caos22_size = sizeof(dump_caos22),
        .rom_caos31 = dump_caos31,
        .rom_caos31_size = sizeof(dump_caos31),
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
    while (true) {
        kc85_exec(&kc85, 66667);
        clear();
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
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 40; x++) {
                const char* ptr = (const char*) &kc85.ram[4][0x3200 + y*40 + x];
                char chr = *ptr;
                if ((chr < 20) || (chr > 127)) {
                    chr = 0x20;
                }
                mvaddch(y, x, chr);
            }
        }
        refresh();
        usleep(66667);
    }
    endwin();
    return 0;
}