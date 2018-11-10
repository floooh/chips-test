/*
    kc85.c

    Stripped down KC85/2../4 emulator running in a (xterm-256color) terminal,
    rendering the ASCII buffer of the KC85 through curses. Requires a UNIX
    environment to build and run (tested on OSX and Linux).

    Select the KC85 model with cmdline args:

        kc85-ascii type=kc85_2
        kc85-ascii type=kc85_3
        kc85-ascii type=kc85_4

        Default is kc85_4.

    Load a snapshot file:

        kc85-ascii snapshot=abspath

    Insert a module with:

        kc85-ascii mod=modname [mod_image=abspath]

        Where modname is one of:
            - m022  (16 KB RAM expansion)
            - m011  (64 KB RAM expansion)
            - m006  (BASIC module for KC85/2, needs mod_image)
            - m012  (TEXOR module, needs mod_image)
            - m026  (FORTH module, needs mod_image)
            - m027  (ASM IDE module, needs mod_image)

    Provide keyboard input for playback with:
        
        kc85-ascii input=...
*/
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#define COMMON_IMPL
#include "fs.h"
#include "keybuf.h"
#define SOKOL_IMPL
#include "sokol_args.h"
#define CHIPS_IMPL
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
/* module to insert after ROM module image has been loaded */
static kc85_module_type_t delay_insert_module = KC85_MODULE_NONE;

// run the emulator and render-loop at 30fps
#define FRAME_USEC (33333)

// a signal handler for Ctrl-C, for proper cleanup 
static int quit_requested = 0;
static void catch_sigint(int signo) {
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

    // select KC85 model
    kc85_type_t type = KC85_TYPE_4;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc85_2")) {
            type = KC85_TYPE_2;
        }
        else if (sargs_equals("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
    }

    // initialize a KC85/4 emulator instance, we don't need audio 
    // or video output, so don't provide a pixel buffer and
    // audio callback
    kc85_init(&kc85, &(kc85_desc_t){
        .type = type,
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

    bool delay_input = false;
    // snapshot file or rom-module image
    if (sargs_exists("snapshot")) {
        delay_input = true;
        fs_load_file(sargs_value("snapshot"));
    }
    else if (sargs_exists("mod_image")) {
        fs_load_file(sargs_value("mod_image"));
    }
    // check if any modules should be inserted
    if (sargs_exists("mod")) {
        // RAM modules can be inserted immediately, ROM modules
        // only after the ROM image has been loaded
        if (sargs_equals("mod", "m022")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (sargs_equals("mod", "m011")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            // a ROM module
            delay_input = true;
            if (sargs_equals("mod", "m006")) {
                delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (sargs_equals("mod", "m012")) {
                delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (sargs_equals("mod", "m026")) {
                delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (sargs_equals("mod", "m027")) {
                delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    // keyboard input to send to emulator
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
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
    
        // handle file IO
        uint32_t delay_frames = kc85.type == KC85_TYPE_4 ? 90 : 240;
        if (fs_ptr() && (frame_count > delay_frames)) {
            if (sargs_exists("snapshot")) {
                kc85_quickload(&kc85, fs_ptr(), fs_size());
                if (sargs_exists("input")) {
                    keybuf_put(sargs_value("input"));
                }
            }
            else if (sargs_exists("mod_image")) {
                // insert the rom module
                if (delay_insert_module != KC85_MODULE_NONE) {
                    kc85_insert_rom_module(&kc85, 0x08, delay_insert_module, fs_ptr(), fs_size());
                }
                if (sargs_exists("input")) {
                    keybuf_put(sargs_value("input"));
                }
            }
            fs_free();
        }
        uint8_t key_code;
        if (0 != (key_code = keybuf_get())) {
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
                if (type == KC85_TYPE_4) {
                    // video memory on KC85/4 is 90 degree rotated and
                    // there are 2 color memory banks
                    int irm_bank = (kc85.io84 & 1) * 2;
                    color_byte = kc85.ram[5+irm_bank][x*256 + y*8];
                }
                else {
                    // video memory on KC85/2,/3 is split in a 32x40 part on left side,
                    // and a 8x40 part on right side
                    uint32_t py = y * 8;
                    uint32_t color_offset;
                    if (x < 32) {
                        uint32_t y_offset = (((py>>2)&0x3f)<<5);
                        color_offset = x + y_offset;
                    }
                    else {
                        uint32_t y_offset = (((py>>4)&0x3)<<3) | (((py>>2)&0x3)<<5) | (((py>>6)&0x3)<<7);
                        color_offset = 0x0800 + y_offset + (x & 7);
                    }
                    color_byte = kc85.ram[4][0x2800 + color_offset];
                }
                int color_pair = ((int)(color_byte & 0x7F))+1;
                if (color_pair != cur_color_pair) {
                    attron(COLOR_PAIR(color_pair));
                    cur_color_pair = color_pair;
                }

                // get ASCII code from character buffer at 0xB200
                char chr = (char) kc85.ram[4][0x3200 + y*40 + x];
                // get character code from ASCII buffer
                if ((chr < 32) || (chr > 127)) {
                    chr = 32;
                }
                // padding to get proper aspect ratio
                mvaddch(y, x*2, ' ');
                // on KC85_4 we need to render an ASCII cursor, the 85/2 and /3
                // implement the cursor through a color byte
                if ((type == KC85_TYPE_4) && (x == cursor_x) && (y == cursor_y)) {
                    attron(A_UNDERLINE);
                }
                // character 
                mvaddch(y, x*2+1, chr);
                // cursor off?
                if ((type == KC85_TYPE_4) && (x == cursor_x) && (y == cursor_y)) {
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
