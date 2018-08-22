//------------------------------------------------------------------------------
//  z9001.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#include "common/common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/z9001.h"
#include "roms/z9001-roms.h"

z9001_t z9001;

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
        .width = 2 * Z9001_DISPLAY_WIDTH,
        .height = 2 * Z9001_DISPLAY_HEIGHT,
        .window_title = "Robotron Z9001/KC87",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* one-time application init */
void app_init() {
    gfx_init(Z9001_DISPLAY_WIDTH, Z9001_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    z9001_type_t type = Z9001_TYPE_Z9001;
    if (args_has("type")) {
        if (args_string_compare("type", "kc87")) {
            type = Z9001_TYPE_KC87;
        }
    }
    z9001_init(&z9001, &(z9001_desc_t){
        .type = type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_z9001_os_1 = dump_z9001_os12_1,
        .rom_z9001_os_1_size = sizeof(dump_z9001_os12_1),
        .rom_z9001_os_2 = dump_z9001_os12_2,
        .rom_z9001_os_2_size = sizeof(dump_z9001_os12_2),
        .rom_z9001_basic = dump_z9001_basic_507_511,
        .rom_z9001_basic_size = sizeof(dump_z9001_basic_507_511),
        .rom_z9001_font = dump_z9001_font,
        .rom_z9001_font_size = sizeof(dump_z9001_font),
        .rom_kc87_os = dump_kc87_os_2,
        .rom_kc87_os_size = sizeof(dump_kc87_os_2),
        .rom_kc87_basic = dump_z9001_basic,
        .rom_kc87_basic_size = sizeof(dump_z9001_basic),
        .rom_kc87_font = dump_kc87_font_2,
        .rom_kc87_font_size = sizeof(dump_kc87_font_2)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    z9001_exec(&z9001, clock_frame_time());
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 20) {
        z9001_quickload(&z9001, fs_ptr(), fs_size());
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
    saudio_shutdown();
    gfx_shutdown();
}
