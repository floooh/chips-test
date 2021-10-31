/*
    kc85.c

    KC85/2, /3 and /4.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80x.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/kc85.h"
#include "kc85-roms.h"

static struct {
    kc85_t kc85;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    // module to insert after ROM module image has been loaded
    kc85_module_type_t delay_insert_module;
    #ifdef CHIPS_USE_UI
        ui_z1013_t ui_z1013;
    #endif
} state;

#ifdef CHIPS_USE_UI
#define BORDER_TOP (24)
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (16)

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// a callback to patch some known problems in game snapshot files
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    (void)user_data;
    if (strcmp(snapshot_name, "JUNGLE     ") == 0) {
        /* patch start level 1 into memory */
        mem_wr(&state.kc85.mem, 0x36b7, 1);
        mem_wr(&state.kc85.mem, 0x3697, 1);
        for (int i = 0; i < 5; i++) {
            mem_wr(&state.kc85.mem, 0x1770 + i, mem_rd(&state.kc85.mem, 0x36b6 + i));
        }
    }
    else if (strcmp(snapshot_name, "DIGGER  COM\x01") == 0) {
        mem_wr16(&state.kc85.mem, 0x09AA, 0x0160);    /* time for delay-loop 0160 instead of 0260 */
        mem_wr(&state.kc85.mem, 0x3d3a, 0xB5);        /* OR L instead of OR (HL) */
    }
    else if (strcmp(snapshot_name, "DIGGERJ") == 0) {
        mem_wr16(&state.kc85.mem, 0x09AA, 0x0260);
        mem_wr(&state.kc85.mem, 0x3d3a, 0xB5);       /* OR L instead of OR (HL) */
    }
}

// get kc85_desc_t struct for a given KC85 type
kc85_desc_t kc85_desc(kc85_type_t type) {
    return (kc85_desc_t) {
        .type = type,
        .pixel_buffer = { .ptr=gfx_framebuffer(), .size=gfx_framebuffer_size() },
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .patch_callback = { .func = patch_snapshots },
        .roms = {
            .caos22 = { .ptr = dump_caos22_852, .size = sizeof(dump_caos22_852) },
            .caos31 = { .ptr = dump_caos31_853, .size = sizeof(dump_caos31_853) },
            .caos42c = { .ptr = dump_caos42c_854, .size = sizeof(dump_caos42c_854) },
            .caos42e = { .ptr = dump_caos42e_854, .size = sizeof(dump_caos42e_854) },
            .kcbasic = { .ptr = dump_basic_c0_853, .size = sizeof(dump_basic_c0_853) }
        }
    };
}

void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border_left = BORDER_LEFT,
        .border_right = BORDER_RIGHT,
        .border_top = BORDER_TOP,
        .border_bottom = BORDER_BOTTOM,
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames = 10 });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
    kc85_type_t type = KC85_TYPE_2;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc85_3")) {
            type = KC85_TYPE_3;
        }
        else if (sargs_equals("type", "kc85_4")) {
            type = KC85_TYPE_4;
        }
    }
    kc85_desc_t desc = kc85_desc(type);
    kc85_init(&state.kc85, &desc);
    #ifdef CHIPS_USE_UI
    // FIXME!
    #endif

    bool delay_input = false;
    // snapshot file or rom-module image
    if (sargs_exists("file")) {
        delay_input=true;
        fs_start_load_file(sargs_value("file"));
    }
    else if (sargs_exists("mod_image")) {
        fs_start_load_file(sargs_value("mod_image"));
    }
    // check if any modules should be inserted
    if (sargs_exists("mod")) {
        /* RAM modules can be inserted immediately, ROM modules
           only after the ROM image has been loaded
        */
        if (sargs_equals("mod", "m022")) {
            kc85_insert_ram_module(&state.kc85, 0x08, KC85_MODULE_M022_16KBYTE);
        }
        else if (sargs_equals("mod", "m011")) {
            kc85_insert_ram_module(&state.kc85, 0x08, KC85_MODULE_M011_64KBYE);
        }
        else {
            // a ROM module
            delay_input = true;
            if (sargs_equals("mod", "m006")) {
                state.delay_insert_module = KC85_MODULE_M006_BASIC;
            }
            else if (sargs_equals("mod", "m012")) {
                state.delay_insert_module = KC85_MODULE_M012_TEXOR;
            }
            else if (sargs_equals("mod", "m026")) {
                state.delay_insert_module = KC85_MODULE_M026_FORTH;
            }
            else if (sargs_equals("mod", "m027")) {
                state.delay_insert_module = KC85_MODULE_M027_DEVELOPMENT;
            }
        }
    }
    // keyboard input to send to emulator
    if (!delay_input) {
        if (sargs_exists("input")) {
            keybuf_put(sargs_value("input"));
        }
    }
}

static void handle_file_loading(void);
static void send_keybuf_input(void);
static void draw_status_bar(void);

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = kc85_exec(&state.kc85, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(kc85_display_width(&state.kc85), kc85_display_height(&state.kc85));
    handle_file_loading();
    send_keybuf_input();
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
                kc85_key_down(&state.kc85, c);
                kc85_key_up(&state.kc85, c);
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
                    kc85_key_down(&state.kc85, c);
                }
                else {
                    kc85_key_up(&state.kc85, c);
                }
            }
            break;
        case SAPP_EVENTTYPE_FILES_DROPPED:
            fs_start_load_dropped_file();
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    kc85_discard(&state.kc85);
    #ifdef CHIPS_USE_UI
    // FIXME
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        kc85_key_down(&state.kc85, key_code);
        kc85_key_up(&state.kc85, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = state.kc85.type == KC85_TYPE_4 ? 180 : 480;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (sargs_exists("mod_image")) {
            // insert the rom module
            if (state.delay_insert_module != KC85_MODULE_NONE) {
                load_success = kc85_insert_rom_module(&state.kc85, 0x08, state.delay_insert_module, fs_ptr(), fs_size());
            }
        }
        else if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = kc85_quickload(&state.kc85, fs_ptr(), fs_size());
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
}

static void draw_status_bar(void) {
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    double frame_time_ms = state.frame_time_us / 1000.0f;
    const char* kc_type;
    switch (state.kc85.type) {
        case KC85_TYPE_2: kc_type = "KC85/2"; break;
        case KC85_TYPE_3: kc_type = "KC85/3"; break;
        case KC85_TYPE_4: kc_type = "KC85/4"; break;
        default: kc_type = "???"; break;
    }
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("%s frame:%.2fms emu:%.2fms ticks/frame:%d",
        kc_type,
        frame_time_ms,
        state.exec_time_ms,
        state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * kc85_std_display_width()  + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * kc85_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "KC85",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}

