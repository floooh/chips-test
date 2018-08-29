/*
    kc85.c

    KC85/2, /3 and /4.
*/
#include "common/common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "roms/kc85-roms.h"

kc85_t kc85;

/* if true, wait with sending text input until file is loaded */
bool delay_input = false;
/* module to insert after ROM module image has been loaded */
kc85_module_type_t delay_insert_module = KC85_MODULE_NONE;

/* sokol-app entry, configure application callbacks and window */
void app_init(void);
void app_frame(void);
void app_input(const sapp_event*);
void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    args_init(argc, argv);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * KC85_DISPLAY_WIDTH,
        .height = 2 * KC85_DISPLAY_HEIGHT,
        .window_title = "KC85",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

/* a callback to patch some known problems in game snapshot files */
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    if (strcmp(snapshot_name, "JUNGLE     ") == 0) {
        /* patch start level 1 into memory */
        mem_wr(&kc85.mem, 0x36b7, 1);
        mem_wr(&kc85.mem, 0x3697, 1);
        for (int i = 0; i < 5; i++) {
            mem_wr(&kc85.mem, 0x1770 + i, mem_rd(&kc85.mem, 0x36b6 + i));
        }
    }
    else if (strcmp(snapshot_name, "DIGGER  COM\x01") == 0) {
        mem_wr16(&kc85.mem, 0x09AA, 0x0160);    /* time for delay-loop 0160 instead of 0260 */
        mem_wr(&kc85.mem, 0x3d3a, 0xB5);        /* OR L instead of OR (HL) */
    }
    else if (strcmp(snapshot_name, "DIGGERJ") == 0) {
        mem_wr16(&kc85.mem, 0x09AA, 0x0260);
        mem_wr(&kc85.mem, 0x3d3a, 0xB5);       /* OR L instead of OR (HL) */
    }
}

void app_init(void) {
    gfx_init(KC85_DISPLAY_WIDTH, KC85_DISPLAY_HEIGHT, 1, 1);
    keybuf_init(6);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    /* specific KC85 model? */
    kc85_type_t type = KC85_TYPE_2;
    if (args_has("type")) {
        if (args_string_compare("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
        else if (args_string_compare("type", "kc85_4")) {
            type = KC85_TYPE_4;
        }
    }
    /* initialize the KC85 emulator */
    kc85_init(&kc85, &(kc85_desc_t){
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .patch_cb = patch_snapshots,
        .rom_caos22 = dump_caos22,
        .rom_caos22_size = sizeof(dump_caos22),
        .rom_caos31 = dump_caos31,
        .rom_caos31_size = sizeof(dump_caos31),
        .rom_caos42c = dump_caos42c,
        .rom_caos42c_size = sizeof(dump_caos42c),
        .rom_caos42e = dump_caos42e,
        .rom_caos42e_size = sizeof(dump_caos42e),
        .rom_kcbasic = dump_basic_c0,
        .rom_kcbasic_size = sizeof(dump_basic_c0)
    });
    /* snapshot file or rom-module image */
    if (args_has("snapshot")) {
        delay_input=true;
        fs_load_file(args_string("snapshot"));
    }
    else if (args_has("mod_image")) {
        fs_load_file(args_string("mod_image"));
    }
    /* check if any modules should be inserted */
    if (args_has("mod")) {
        /* RAM modules can be inserted immediately, ROM modules
           only after the ROM image has been loaded
        */
        if (args_string_compare("mod", "m022")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (args_string_compare("mod", "m011")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            /* a ROM module */
            delay_input = true;
            if (args_string_compare("mod", "m006")) {
                delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (args_string_compare("mod", "m012")) {
                delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (args_string_compare("mod", "m026")) {
                delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (args_string_compare("mod", "m027")) {
                delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    /* keyboard input to send to emulator */
    if (!delay_input) {
        if (args_has("input")) {
            keybuf_put(args_string("input"));
        }
    }
}

void app_frame(void) {
    kc85_exec(&kc85, clock_frame_time());
    gfx_draw();
    uint32_t delay_frames = kc85.type == KC85_TYPE_4 ? 180 : 480;
    if (fs_ptr() && clock_frame_count() > delay_frames) {
        if (args_has("snapshot")) {
            kc85_quickload(&kc85, fs_ptr(), fs_size());
            if (args_has("input")) {
                keybuf_put(args_string("input"));
            }
        }
        else if (args_has("mod_image")) {
            /* insert the rom module */
            if (delay_insert_module != KC85_MODULE_NONE) {
                kc85_insert_rom_module(&kc85, 0x08, delay_insert_module, fs_ptr(), fs_size());
            }
            if (args_has("input")) {
                keybuf_put(args_string("input"));
            }
        }
        fs_free();
    }
    uint8_t key_code;
    if (0 != (key_code = keybuf_get())) {
        kc85_key_down(&kc85, key_code);
        kc85_key_up(&kc85, key_code);
    }

}

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
                kc85_key_down(&kc85, c);
                kc85_key_up(&kc85, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = shift ? 0x5B : 0x20; break; /* 0x5B: neg space, 0x20: space */
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_HOME:         c = 0x10; break;
                case SAPP_KEYCODE_INSERT:       c = shift ? 0x0C : 0x1A; break; /* 0x0C: cls, 0x1A: ins */
                case SAPP_KEYCODE_BACKSPACE:    c = shift ? 0x0C : 0x01; break; /* 0x0C: cls, 0x01: del */ 
                case SAPP_KEYCODE_ESCAPE:       c = shift ? 0x13 : 0x03; break; /* 0x13: stop, 0x03: brk */
                case SAPP_KEYCODE_F1:           c = 0xF1; break;
                case SAPP_KEYCODE_F2:           c = 0xF2; break;
                case SAPP_KEYCODE_F3:           c = 0xF3; break;
                case SAPP_KEYCODE_F4:           c = 0xF4; break;
                case SAPP_KEYCODE_F5:           c = 0xF5; break;
                case SAPP_KEYCODE_F6:           c = 0xF6; break;
                case SAPP_KEYCODE_F7:           c = 0xF7; break;
                case SAPP_KEYCODE_F8:           c = 0xF8; break;
                case SAPP_KEYCODE_F9:           c = 0xF9; break;
                case SAPP_KEYCODE_F10:          c = 0xFA; break;
                case SAPP_KEYCODE_F11:          c = 0xFB; break;
                case SAPP_KEYCODE_F12:          c = 0xFC; break;
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    kc85_key_down(&kc85, c);
                }
                else {
                    kc85_key_up(&kc85, c);
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

void app_cleanup(void) {
    kc85_discard(&kc85);
    saudio_shutdown();
    gfx_shutdown();
}
