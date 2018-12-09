/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - wait states when accessing contended memory are not emulated
    - video decoding works with scanline accuracy, not cycle accuracy
    - no tape or disc emulation
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
#include "zx-roms.h"

/* imports from zx-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void zxui_init(zx_t* zx);
void zxui_discard(void);
void zxui_draw(void);
void zxui_set_exec_time(double t);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

static zx_t zx;

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
        .width = 2 * zx_std_display_width(),
        .height = 2 * zx_std_display_height() + ui_extra_height,
        .window_title = "ZX Spectrum",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* get zx_desc_t struct for given ZX type and joystick type */
zx_desc_t zx_desc(zx_type_t type, zx_joystick_type_t joy_type) {
    return (zx_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_zx48k = dump_amstrad_zx48k,
        .rom_zx48k_size = sizeof(dump_amstrad_zx48k),
        .rom_zx128_0 = dump_amstrad_zx128k_0,
        .rom_zx128_0_size = sizeof(dump_amstrad_zx128k_0),
        .rom_zx128_1 = dump_amstrad_zx128k_1,
        .rom_zx128_1_size = sizeof(dump_amstrad_zx128k_1)
    };
}

/* one-time application init */
void app_init() {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        .top_offset = ui_extra_height,
        #endif
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    zx_type_t type = ZX_TYPE_128;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "zx48k")) {
            type = ZX_TYPE_48K;
        }
    }
    zx_joystick_type_t joy_type = ZX_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "kempston")) {
            joy_type = ZX_JOYSTICKTYPE_KEMPSTON;
        }
        else if (sargs_equals("joystick", "sinclair1")) {
            joy_type = ZX_JOYSTICKTYPE_SINCLAIR_1;
        }
        else if (sargs_equals("joystick", "sinclair2")) {
            joy_type = ZX_JOYSTICKTYPE_SINCLAIR_2;
        }
    }
    zx_desc_t desc = zx_desc(type, joy_type);
    zx_init(&zx, &desc);
    #ifdef CHIPS_USE_UI
    zxui_init(&zx);
    #endif
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    #if CHIPS_USE_UI
        uint64_t start = stm_now();
        zx_exec(&zx, clock_frame_time());
        zxui_set_exec_time(stm_ms(stm_since(start)));
    #else
        zx_exec(&zx, clock_frame_time());
    #endif
    gfx_draw(zx_display_width(&zx), zx_display_height(&zx));
    if (fs_ptr() && clock_frame_count() > 120) {
        zx_quickload(&zx, fs_ptr(), fs_size());
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
            if ((c > 0x20) && (c < 0x7F)) {
                zx_key_down(&zx, c);
                zx_key_up(&zx, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x0C; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x07; break;
                case SAPP_KEYCODE_LEFT_CONTROL: c = 0x0F; break; /* SymShift */
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    zx_key_down(&zx, c);
                }
                else {
                    zx_key_up(&zx, c);
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
    zx_discard(&zx);
    #ifdef CHIPS_USE_UI
    zxui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}
