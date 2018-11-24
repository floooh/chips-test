/*
    z1013.c

    The Robotron Z1013, see chips/systems/z1013.h for details!

    I have added a pre-loaded KC-BASIC interpreter:

    Start the BASIC interpreter with 'J 300', return to OS with 'BYE'.

    Enter BASIC editing mode with 'AUTO', leave by pressing Esc.
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
#ifdef CHIPS_USE_UI
#undef CHIPS_IMPL
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z1013.h"
#endif

static z1013_t z1013;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

#ifdef CHIPS_USE_UI
static uint64_t exec_time;
static void z1013ui_init(void);
static void z1013ui_discard(void);
static void z1013ui_draw(void);
#endif

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
        .width = 2 * Z1013_DISPLAY_WIDTH,
        .height = 2 * Z1013_DISPLAY_HEIGHT,
        .window_title = "Robotron Z1013",
        .ios_keyboard_resizes_canvas = true
    };
}

/* get z1013_desc_t struct for given Z1013 model */
static z1013_desc_t z1013_desc(z1013_type_t type) {
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
        .fb_width = Z1013_DISPLAY_WIDTH,
        .fb_height = Z1013_DISPLAY_HEIGHT + 16,
        #else
        .fb_width = Z1013_DISPLAY_WIDTH,
        .fb_height = Z1013_DISPLAY_HEIGHT,
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
    z1013ui_init();
    #endif
    /* FIXME: remove BASIC interpreter and use snapshot loading in user-code instead */
    if (Z1013_TYPE_64 == z1013.type) {
        mem_write_range(&z1013.mem, 0x0100, dump_kc_basic+0x20, sizeof(dump_kc_basic)-0x20);
    }
}

/* per frame stuff: tick the emulator, render the framebuffer, delay-load game files */
void app_frame(void) {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        z1013_exec(&z1013, clock_frame_time());
        exec_time = stm_since(start);
    #else
        z1013_exec(&z1013, clock_frame_time());
    #endif
    gfx_draw();
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

/*=== optional debugging UI ==================================================*/
#ifdef CHIPS_USE_UI

static ui_z1013_t ui_z1013;

/* reboot callback */
static void boot_cb(z1013_t* sys, z1013_type_t type) {
    z1013_desc_t desc = z1013_desc(type);
    z1013_init(&z1013, &desc);
}

void z1013ui_init(void) {
    ui_init(z1013ui_draw);
    ui_z1013_init(&ui_z1013, &(ui_z1013_desc_t){
        .z1013 = &z1013,
        .boot_cb = boot_cb
    });
}

void z1013ui_discard(void) {
    ui_z1013_discard(&ui_z1013);
}

void z1013ui_draw() {
    ui_z1013_draw(&ui_z1013, stm_ms(exec_time));
}
#endif /* CHIPS_USE_UI */
