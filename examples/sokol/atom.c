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

/* imports from cpc-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void atomui_init(atom_t* atom);
void atomui_discard(void);
void atomui_draw(void);
void atomui_exec(uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

static atom_t atom;

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
        .width = 2 * atom_std_display_width(),
        .height = 2 * atom_std_display_height() + ui_extra_height,
        .window_title = "Acorn Atom",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

/* get atom_desc_t struct based on joystick type */
atom_desc_t atom_desc(atom_joystick_type_t joy_type) {
    return (atom_desc_t) {
        .joystick_type = joy_type,
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .rom_abasic = dump_abasic_ic20,
        .rom_abasic_size = sizeof(dump_abasic_ic20),
        .rom_afloat = dump_afloat_ic21,
        .rom_afloat_size = sizeof(dump_afloat_ic21),
        .rom_dosrom = dump_dosrom_u15,
        .rom_dosrom_size = sizeof(dump_dosrom_u15)
    };
}

/* one-time application init */
void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .top_offset = ui_extra_height
    });
    keybuf_init(10);
    clock_init();
    bool delay_input = false;
    fs_init();
    if (sargs_exists("file")) {
        delay_input = true;
        if (!fs_load_file(sargs_value("file"))) {
            gfx_flash_error();
        }
    }
    saudio_setup(&(saudio_desc){0});
    atom_joystick_type_t joy_type = ATOM_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "mmc") || sargs_equals("joystick", "yes")) {
            joy_type = ATOM_JOYSTICKTYPE_MMC;
        }
    }
    atom_desc_t desc = atom_desc(joy_type);
    atom_init(&atom, &desc);
    #ifdef CHIPS_USE_UI
    atomui_init(&atom);
    #endif
    /* keyboard input to send to emulator */
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

/* per frame stuff, tick the emulator, handle input, decode and draw emulator display */
void app_frame() {
    const uint32_t frame_time = clock_frame_time();
    #if CHIPS_USE_UI
        atomui_exec(frame_time);
    #else
        atom_exec(&atom, frame_time);
    #endif
    gfx_draw(atom_display_width(&atom), atom_display_height(&atom));
    const uint32_t load_delay_frames = 48;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        if (fs_ext("tap")) {
            load_success = atom_insert_tape(&atom, fs_ptr(), fs_size());
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
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
    if (0 != (key_code = keybuf_get(frame_time))) {
        atom_key_down(&atom, key_code);
        atom_key_up(&atom, key_code);
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
    #ifdef CHIPS_USE_UI
    atomui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}
