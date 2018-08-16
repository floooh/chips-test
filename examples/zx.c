/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - wait states when accessing contended memory are not emulated
    - video decoding works with scanline accuracy, not cycle accuracy
    - no tape or disc emulation
*/
#include "sokol_app.h"
#include "sokol_audio.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/zx.h"
#include "common/gfx.h"
#include "common/clock.h"
#include "common/fs.h"
#include "common/args.h"
#include "roms/zx-roms.h"

zx_t zx;

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
        .width = 2 * ZX_DISPLAY_WIDTH,
        .height = 2 * ZX_DISPLAY_HEIGHT,
        .window_title = "ZX Spectrum",
        .ios_keyboard_resizes_canvas = true
    };
}

/* one-time application init */
void app_init() {
    gfx_init(ZX_DISPLAY_WIDTH, ZX_DISPLAY_HEIGHT, 1, 1);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    zx_type_t type = ZX_TYPE_128;
    if (args_has("type")) {
        if (args_string_compare("type", "zx48k")) {
            type = ZX_TYPE_48K;
        }
    }
    zx_joystick_t joy_type = ZX_JOYSTICK_NONE;
    if (args_has("joystick")) {
        if (args_string_compare("joystick", "kempston")) {
            joy_type = ZX_JOYSTICK_KEMPSTON;
        }
        else if (args_string_compare("joystick", "sinclair1")) {
            joy_type = ZX_JOYSTICK_SINCLAIR_1;
        }
        else if (args_string_compare("joystick", "sinclair2")) {
            joy_type = ZX_JOYSTICK_SINCLAIR_2;
        }
    }
    zx_init(&zx, &(zx_desc_t) {
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = rgba8_buffer,
        .pixel_buffer_size = sizeof(rgba8_buffer),
        .audio_cb = saudio_push,
        .audio_sample_rate = saudio_sample_rate(),
        .rom_zx48k = dump_amstrad_zx48k,
        .rom_zx48k_size = sizeof(dump_amstrad_zx48k),
        .rom_zx128_0 = dump_amstrad_zx128k_0,
        .rom_zx128_0_size = sizeof(dump_amstrad_zx128k_0),
        .rom_zx128_1 = dump_amstrad_zx128k_1,
        .rom_zx128_1_size = sizeof(dump_amstrad_zx128k_1)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    zx_exec(&zx, clock_frame_time());
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 120) {
        zx_quickload(&zx, fs_ptr(), fs_size());
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < KBD_MAX_KEYS)) {
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
    saudio_shutdown();
    gfx_shutdown();
}
