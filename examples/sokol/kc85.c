/*
    kc85.c

    KC85/2, /3 and /4.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "kc85-roms.h"

/* imports from kc85-ui.cc */
#ifdef CHIPS_USE_UI
#include "ui.h"
void kc85ui_init(kc85_t* kc85);
void kc85ui_discard(void);
void kc85ui_draw(void);
void kc85ui_exec(kc85_t* kc85, uint32_t frame_time_us);
static const int ui_extra_height = 16;
#else
static const int ui_extra_height = 0;
#endif

static kc85_t kc85;

/* module to insert after ROM module image has been loaded */
kc85_module_type_t delay_insert_module = KC85_MODULE_NONE;

/* sokol-app entry, configure application callbacks and window */
static void app_init(void);
static void app_frame(void);
static void app_input(const sapp_event*);
static void app_cleanup(void);

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * kc85_std_display_width(),
        .height = 2 * kc85_std_display_height() + ui_extra_height,
        .window_title = "KC85",
        .ios_keyboard_resizes_canvas = true
    };
}

/* audio-streaming callback */
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

/* a callback to patch some known problems in game snapshot files */
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    (void)user_data;
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

/* get kc85_desc_t struct for a given KC85 type */
kc85_desc_t kc85_desc(kc85_type_t type) {
    return (kc85_desc_t) {
        .type = type,
        .pixel_buffer = gfx_framebuffer(),
        .pixel_buffer_size = gfx_framebuffer_size(),
        .audio_cb = push_audio,
        .audio_sample_rate = saudio_sample_rate(),
        .patch_cb = patch_snapshots,
        .rom_caos22 = dump_caos22_852,
        .rom_caos22_size = sizeof(dump_caos22_852),
        .rom_caos31 = dump_caos31_853,
        .rom_caos31_size = sizeof(dump_caos31_853),
        .rom_caos42c = dump_caos42c_854,
        .rom_caos42c_size = sizeof(dump_caos42c_854),
        .rom_caos42e = dump_caos42e_854,
        .rom_caos42e_size = sizeof(dump_caos42e_854),
        .rom_kcbasic = dump_basic_c0_853,
        .rom_kcbasic_size = sizeof(dump_basic_c0_853)
    };
}

void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .top_offset = ui_extra_height
    });
    keybuf_init(10);
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    /* specific KC85 model? */
    kc85_type_t type = KC85_TYPE_2;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
        else if (sargs_equals("type", "kc85_4")) {
            type = KC85_TYPE_4;
        }
    }
    /* initialize the KC85 emulator */
    kc85_desc_t desc = kc85_desc(type);
    kc85_init(&kc85, &desc);
    #ifdef CHIPS_USE_UI
    kc85ui_init(&kc85);
    #endif

    bool delay_input = false;
    /* snapshot file or rom-module image */
    if (sargs_exists("file")) {
        delay_input=true;
        if (!fs_load_file(sargs_value("file"))) {
            gfx_flash_error();
        }
    }
    else if (sargs_exists("mod_image")) {
        if (!fs_load_file(sargs_value("mod_image"))) {
            gfx_flash_error();
        }
    }
    /* check if any modules should be inserted */
    if (sargs_exists("mod")) {
        /* RAM modules can be inserted immediately, ROM modules
           only after the ROM image has been loaded
        */
        if (sargs_equals("mod", "m022")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (sargs_equals("mod", "m011")) {
            kc85_insert_ram_module(&kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            /* a ROM module */
            delay_input = true;
            if (sargs_equals("mod", "m006")) {
                delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (sargs_equals("mod", "m012")) {
                delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (sargs_equals("mod", "m026")) {
                delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (sargs_equals("mod", "m027")) {
                delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    /* keyboard input to send to emulator */
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

void app_frame(void) {
    const uint32_t frame_time = clock_frame_time();
    #if CHIPS_USE_UI
        kc85ui_exec(&kc85, frame_time);
    #else
        kc85_exec(&kc85, frame_time);
    #endif
    gfx_draw(kc85_display_width(&kc85), kc85_display_height(&kc85));
    const uint32_t load_delay_frames = kc85.type == KC85_TYPE_4 ? 180 : 480;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (sargs_exists("mod_image")) {
            /* insert the rom module */
            if (delay_insert_module != KC85_MODULE_NONE) {
                load_success = kc85_insert_rom_module(&kc85, 0x08, delay_insert_module, fs_ptr(), fs_size());
            }
        }
        else if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = kc85_quickload(&kc85, fs_ptr(), fs_size());
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
        kc85_key_down(&kc85, key_code);
        kc85_key_up(&kc85, key_code);
    }
}

void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
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
    #ifdef CHIPS_USE_UI
    kc85ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}
