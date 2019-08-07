//------------------------------------------------------------------------------
//  z9001.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/z9001.h"
#include "z9001-roms.h"

/* imports from z9001-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void z9001ui_init(z9001_t* z9001);
void z9001ui_discard(void);
void z9001ui_draw(void);
void z9001ui_exec(z9001_t* z9001, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

static z9001_t z9001;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * z9001_std_display_width(),
        .height = 2 * z9001_std_display_height() + ui_extra_height,
        .window_title = "Robotron Z9001/KC87",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get a z9001_desc_t struct for given Z9001 model */
z9001_desc_t z9001_desc(z9001_type_t type) {
    return (z9001_desc_t) {
        .type = type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_z9001_os_1 = dump_z9001_os12_1_bin,
        .rom_z9001_os_1_size = sizeof(dump_z9001_os12_1_bin),
        .rom_z9001_os_2 = dump_z9001_os12_2_bin,
        .rom_z9001_os_2_size = sizeof(dump_z9001_os12_2_bin),
        .rom_z9001_basic = dump_z9001_basic_507_511_bin,
        .rom_z9001_basic_size = sizeof(dump_z9001_basic_507_511_bin),
        .rom_z9001_font = dump_z9001_font_bin,
        .rom_z9001_font_size = sizeof(dump_z9001_font_bin),
        .rom_kc87_os = dump_kc87_os_2_bin,
        .rom_kc87_os_size = sizeof(dump_kc87_os_2_bin),
        .rom_kc87_basic = dump_z9001_basic_bin,
        .rom_kc87_basic_size = sizeof(dump_z9001_basic_bin),
        .rom_kc87_font = dump_kc87_font_2_bin,
        .rom_kc87_font_size = sizeof(dump_kc87_font_2_bin)
    };
}

/* one-time application init */
void app_init() {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .top_offset = ui_extra_height
    });
    keybuf_init(12);
    clock_init();
    fs_init();
    saudio_setup(&(saudio_desc){0});
    z9001_type_t type = Z9001_TYPE_Z9001;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc87")) {
            type = Z9001_TYPE_KC87;
        }
    }
    z9001_desc_t desc = z9001_desc(type);
    z9001_init(&z9001, &desc);
    #ifdef CHIPS_USE_UI
    z9001ui_init(&z9001);
    #endif
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        if (!fs_load_file(sargs_value("file"))) {
            gfx_flash_error();
        }
    }
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    #if CHIPS_USE_UI
        z9001ui_exec(&z9001, clock_frame_time());
    #else
        z9001_exec(&z9001, clock_frame_time());
    #endif
    gfx_draw(z9001_display_width(&z9001), z9001_display_height(&z9001));
    if (fs_ptr() && clock_frame_count() > 20) {
        bool load_success = false;
        if (fs_ext("txt") || (fs_ext("bas"))) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = z9001_quickload(&z9001, fs_ptr(), fs_size());
        }
        if (load_success) {
            gfx_flash_success();
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        }
        else {
            gfx_flash_error();
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        z9001_key_down(&z9001, key_code);
        z9001_key_up(&z9001, key_code);
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
                z9001_key_down(&z9001, c);
                z9001_key_up(&z9001, c);
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
                case SAPP_KEYCODE_ESCAPE:   c = (event->modifiers & SAPP_MODIFIER_SHIFT)? 0x1B: 0x03; break;
                case SAPP_KEYCODE_INSERT:   c = 0x1A; break;
                case SAPP_KEYCODE_HOME:     c = 0x19; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    z9001_key_down(&z9001, c);
                }
                else {
                    z9001_key_up(&z9001, c);
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
void app_cleanup() {
    z9001_discard(&z9001);
    #ifdef CHIPS_USE_UI
    z9001ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}
