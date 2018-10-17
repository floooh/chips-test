/*
    c64.c
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c64.h"
#include "c64-roms.h"

c64_t c64;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * C64_DISPLAY_WIDTH,
        .height = 2 * C64_DISPLAY_HEIGHT,
        .window_title = "C64",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t){
        .fb_width = C64_DISPLAY_WIDTH,
        .fb_height = C64_DISPLAY_HEIGHT
    });
    keybuf_init(10);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    if (sargs_exists("tape")) {
        fs_load_file(sargs_value("tape"));
    }
    c64_joystick_type_t joy_type = C64_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "digital_1")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_1;
        }
        else if (sargs_equals("joystick", "digital_2")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_2;
        }
    }
    c64_init(&c64, &(c64_desc_t){
        .joystick_type = joy_type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .audio_tape_sound = sargs_boolean("tape_sound"),
        .rom_char = dump_c64_char,
        .rom_char_size = sizeof(dump_c64_char),
        .rom_basic = dump_c64_basic,
        .rom_basic_size = sizeof(dump_c64_basic),
        .rom_kernal = dump_c64_kernalv3,
        .rom_kernal_size = sizeof(dump_c64_kernalv3)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame(void) {
    c64_exec(&c64, clock_frame_time());
    gfx_draw();
    if (fs_ptr() && clock_frame_count() > 180) {
        if (c64_insert_tape(&c64, fs_ptr(), fs_size())) {
            /* send load command */
            keybuf_put("LOAD\n");
            c64_start_tape(&c64);
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        /* FIXME: this is ugly */
        c64_joystick_type_t joy_type = c64.joystick_type;
        c64.joystick_type = C64_JOYSTICKTYPE_NONE;
        c64_key_down(&c64, key_code);
        c64_key_up(&c64, key_code);
        c64.joystick_type = joy_type;
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                c64_key_down(&c64, c);
                c64_key_up(&c64, c);
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
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break;
                case SAPP_KEYCODE_F1:           c = 0xF1; break;
                case SAPP_KEYCODE_F2:           c = 0xF2; break;
                case SAPP_KEYCODE_F3:           c = 0xF3; break;
                case SAPP_KEYCODE_F4:           c = 0xF4; break;
                case SAPP_KEYCODE_F5:           c = 0xF5; break;
                case SAPP_KEYCODE_F6:           c = 0xF6; break;
                case SAPP_KEYCODE_F7:           c = 0xF7; break;
                case SAPP_KEYCODE_F8:           c = 0xF8; break;
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    c64_key_down(&c64, c);
                }
                else {
                    c64_key_up(&c64, c);
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
    c64_discard(&c64);
    saudio_shutdown();
    gfx_shutdown();
}
