/*
    z1013.c

    The Robotron Z1013, see chips/systems/z1013.h for details!
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/z1013.h"
#include "z1013-roms.h"

/* imports from z1013-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void z1013ui_init(z1013_t* z1013);
void z1013ui_discard(void);
void z1013ui_draw(void);
void z1013ui_exec(z1013_t* z1013, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

static z1013_t z1013;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    fs_init();
    if (sargs_exists("file")) {
        fs_load_file(sargs_value("file"));
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * z1013_std_display_width(),
        .height = 2 * z1013_std_display_height() + ui_extra_height,
        .window_title = "Robotron Z1013",
        .ios_keyboard_resizes_canvas = true
    };
}

/* get z1013_desc_t struct for given Z1013 model */
z1013_desc_t z1013_desc(z1013_type_t type) {
    return(z1013_desc_t) {
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_mon_a2 = dump_z1013_mon_a2,
        .rom_mon_a2_size = sizeof(dump_z1013_mon_a2),
        .rom_mon202 = dump_z1013_mon202,
        .rom_mon202_size = sizeof(dump_z1013_mon202),
        .rom_font = dump_z1013_font,
        .rom_font_size = sizeof(dump_z1013_font)
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = 16,
        #endif
    });
    clock_init();
    z1013_type_t type = Z1013_TYPE_64;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "z1013_01")) {
            type = Z1013_TYPE_01;
        }
        else if (sargs_equals("type", "z1013_16")) {
            type = Z1013_TYPE_16;
        }
    }
    z1013_desc_t desc = z1013_desc(type);
    z1013_init(&z1013, &desc);
    #ifdef CHIPS_USE_UI
    z1013ui_init(&z1013);
    #endif
}

/* per frame stuff: tick the emulator, render the framebuffer, delay-load game files */
void app_frame(void) {
    #if CHIPS_USE_UI
        z1013ui_exec(&z1013, clock_frame_time());
    #else
        z1013_exec(&z1013, clock_frame_time());
    #endif
    gfx_draw(z1013_display_width(&z1013), z1013_display_height(&z1013));
    if (fs_ptr() && clock_frame_count() > 20) {
        z1013_quickload(&z1013, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c >= 0x20) && (c < 0x7F)) {
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
    #ifdef CHIPS_USE_UI
    z1013ui_discard();
    #endif
    gfx_shutdown();
    sargs_shutdown();
}
