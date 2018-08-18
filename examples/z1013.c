/*
    z1013.c

    The Robotron Z1013, see chips/systems/z1013.h for details!

    I have added a pre-loaded KC-BASIC interpreter:

    Start the BASIC interpreter with 'J 300', return to OS with 'BYE'.

    Enter BASIC editing mode with 'AUTO', leave by pressing Esc.
*/
#include "sokol_app.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
#include "common/common.h"
#include "roms/z1013-roms.h"
#include <ctype.h> /* isupper, islower, toupper, tolower */

static z1013_t z1013;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    fs_init();
    if (args_has("file")) {
        fs_load_file(args_string("file"));
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * Z1013_DISPLAY_WIDTH,
        .height = 2 * Z1013_DISPLAY_HEIGHT,
        .window_title = "Robotron Z1013",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(Z1013_DISPLAY_WIDTH, Z1013_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    z1013_type_t type = Z1013_TYPE_64;
    if (args_has("type")) {
        if (args_string_compare("type", "z1013_01")) {
            type = Z1013_TYPE_01;
        }
        else if (args_string_compare("type", "z1013_16")) {
            type = Z1013_TYPE_16;
        }
    }
    z1013_init(&z1013, &(z1013_desc_t) {
        .type = type,
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .rom_mon_a2 = dump_z1013_mon_a2,
        .rom_mon_a2_size = sizeof(dump_z1013_mon_a2),
        .rom_mon202 = dump_z1013_mon202,
        .rom_mon202_size = sizeof(dump_z1013_mon202),
        .rom_font = dump_z1013_font,
        .rom_font_size = sizeof(dump_z1013_font)
    });
    /* copy BASIC interpreter into RAM, so the user has something to play around 
        with (started with "J 300 [Enter]"), skip first 0x20 bytes .z80 file format header
    */
    if (Z1013_TYPE_64 == z1013.type) {
        mem_write_range(&z1013.mem, 0x0100, dump_kc_basic+0x20, sizeof(dump_kc_basic)-0x20);
    }
}

/* per frame stuff: tick the emulator, render the framebuffer, delay-load game files */
void app_frame(void) {
    z1013_exec(&z1013, clock_frame_time());
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 20) {
        z1013_quickload(&z1013, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if (c < KBD_MAX_KEYS) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                z1013_key_down(&z1013, c);
                z1013_key_up(&z1013, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_ENTER:    c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:    c = 0x09; break;
                case SAPP_KEYCODE_LEFT:     c = 0x08; break;
                case SAPP_KEYCODE_DOWN:     c = 0x0A; break;
                case SAPP_KEYCODE_UP:       c = 0x0B; break;
                case SAPP_KEYCODE_ESCAPE:   c = 0x03; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    z1013_key_down(&z1013, c);
                }
                else {
                    z1013_key_up(&z1013, c);
                }
            }
            break;
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            sapp_show_keyboard(true);
            break;
        default:
            break;
    }
}

/* application cleanup callback */
void app_cleanup(void) {
    z1013_discard(&z1013);
    gfx_shutdown();
}
