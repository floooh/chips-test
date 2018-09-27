/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    (just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip).

    Note: Ctrl+L (clear screen) is mapped to F1.

    NOT EMULATED:
        - REPT key (and some other special keys)
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#include "atom-roms.h"

atom_t atom;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);
sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    fs_init();
    if (args_has("tape")) {
        fs_load_file(args_string("tape"));
    }
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * ATOM_DISPLAY_WIDTH,
        .height = 2 * ATOM_DISPLAY_HEIGHT,
        .window_title = "Acorn Atom",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        .fb_width = ATOM_DISPLAY_WIDTH,
        .fb_height = ATOM_DISPLAY_HEIGHT
    });
    keybuf_init(10);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    atom_joystick_type_t joy_type = ATOM_JOYSTICKTYPE_NONE;
    if (args_has("joystick")) {
        if (args_string_compare("joystick", "mmc") ||
            args_string_compare("joystick", "yes"))
        {
            joy_type = ATOM_JOYSTICKTYPE_MMC;
        }
    }
    atom_init(&atom, &(atom_desc_t){
        .joystick_type = joy_type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_abasic = dump_abasic,
        .rom_abasic_size = sizeof(dump_abasic),
        .rom_afloat = dump_afloat,
        .rom_afloat_size = sizeof(dump_afloat),
        .rom_dosrom = dump_dosrom,
        .rom_dosrom_size = sizeof(dump_dosrom)
    });
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    atom_exec(&atom, clock_frame_time());
    gfx_draw();
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        atom_key_down(&atom, key_code);
        atom_key_up(&atom, key_code);
    }
    if (fs_ptr() && clock_frame_count() > 48) {
        if (atom_insert_tape(&atom, fs_ptr(), fs_size())) {
            keybuf_put("*LOAD\n\n");
        }
        fs_free();
    }
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    int c = 0;
    switch (event->type) {
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
                atom_key_down(&atom, c);
                atom_key_up(&atom, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_INSERT:       c = 0x1A; break;
                case SAPP_KEYCODE_HOME:         c = 0x19; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x1B; break;
                case SAPP_KEYCODE_F1:           c = 0x0C; break; /* mapped to Ctrl+L (clear screen) */
                default:                        c = 0;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    atom_key_down(&atom, c);
                }
                else {
                    atom_key_up(&atom, c);
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
    atom_discard(&atom);
    saudio_shutdown();
    gfx_shutdown();
}
