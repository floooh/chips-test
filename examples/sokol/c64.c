/*
    c64.c
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/m6502.h"
#include "chips/m6526.h"
#include "chips/m6569.h"
#include "chips/m6581.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/c1530.h"
#include "chips/m6522.h"
#include "systems/c1541.h"
#include "systems/c64.h"
#include "c64-roms.h"
#include "c1541-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_m6526.h"
    #include "ui/ui_m6581.h"
    #include "ui/ui_m6569.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_c64.h"
#endif

static struct {
    c64_t c64;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #ifdef CHIPS_USE_UI
        ui_c64_t ui_c64;
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

// get c64_desc_t struct based on joystick type
c64_desc_t c64_desc(c64_joystick_type_t joy_type, bool c1530_enabled, bool c1541_enabled) {
    return (c64_desc_t) {
        .c1530_enabled = c1530_enabled,
        .c1541_enabled = c1541_enabled,
        .joystick_type = joy_type,
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .pixel_buffer = {
            .ptr = gfx_framebuffer(),
            .size = gfx_framebuffer_size(),
        },
        .roms = {
            .chars = { .ptr=dump_c64_char_bin, .size=sizeof(dump_c64_char_bin) },
            .basic = { .ptr=dump_c64_basic_bin, .size=sizeof(dump_c64_basic_bin) },
            .kernal = { .ptr=dump_c64_kernalv3_bin, .size=sizeof(dump_c64_kernalv3_bin) },
            .c1541 = {
                .c000_dfff = { .ptr=dump_1541_c000_325302_01_bin, .size=sizeof(dump_1541_c000_325302_01_bin) },
                .e000_ffff = { .ptr=dump_1541_e000_901229_06aa_bin, .size=sizeof(dump_1541_e000_901229_06aa_bin) },
            }
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_c64_get_debug(&state.ui_c64)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_c64_draw(&state.ui_c64);
}
static void ui_boot_cb(c64_t* sys) {
    c64_desc_t desc = c64_desc(sys->joystick_type, sys->c1530.valid, sys->c1541.valid);
    c64_init(sys, &desc);
}
#endif

void app_init(void) {
    gfx_init(&(gfx_desc_t){
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border_left = BORDER_LEFT,
        .border_right = BORDER_RIGHT,
        .border_top = BORDER_TOP,
        .border_bottom = BORDER_BOTTOM,
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames=5 });
    clock_init();
    prof_init();
    fs_init();
    saudio_setup(&(saudio_desc){0});
    c64_joystick_type_t joy_type = C64_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "digital_1")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_1;
        }
        else if (sargs_equals("joystick", "digital_2")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_2;
        }
        else if (sargs_equals("joystick", "digital_12")) {
            joy_type = C64_JOYSTICKTYPE_DIGITAL_12;
        }
    }
    bool c1530_enabled = sargs_exists("c1530");
    bool c1541_enabled = sargs_exists("c1541");
    c64_desc_t desc = c64_desc(joy_type, c1530_enabled, c1541_enabled);
    c64_init(&state.c64, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_c64_init(&state.ui_c64, &(ui_c64_desc_t){
            .c64 = &state.c64,
            .boot_cb = ui_boot_cb,
            .create_texture_cb = gfx_create_texture,
            .update_texture_cb = gfx_update_texture,
            .destroy_texture_cb = gfx_destroy_texture,
            .dbg_keys = {
                .cont = { .keycode = SAPP_KEYCODE_F5, .name = "F5" },
                .stop = { .keycode = SAPP_KEYCODE_F5, .name = "F5" },
                .step_over = { .keycode = SAPP_KEYCODE_F6, .name = "F6" },
                .step_into = { .keycode = SAPP_KEYCODE_F7, .name = "F7" },
                .step_tick = { .keycode = SAPP_KEYCODE_F8, .name = "F8" },
                .toggle_breakpoint = { .keycode = SAPP_KEYCODE_F9, .name = "F9" }
            }
        });
    #endif
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        fs_start_load_file(sargs_value("file"));
    }
    if (sargs_exists("prg")) {
        fs_load_base64("url.prg", sargs_value("prg"));
    }
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
    const uint64_t emu_start_time = stm_now();
    state.ticks = c64_exec(&state.c64, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(c64_display_width(&state.c64), c64_display_height(&state.c64));
    handle_file_loading();
    send_keybuf_input();
}

void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_start_load_dropped_file();
    }
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
        return;
    }
    #endif
    const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                // need to invert case (unshifted is upper caps, shifted is lower caps
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                c64_key_down(&state.c64, c);
                c64_key_up(&state.c64, c);
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
                    c64_key_down(&state.c64, c);
                }
                else {
                    c64_key_up(&state.c64, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    c64_discard(&state.c64);
    #ifdef CHIPS_USE_UI
        ui_c64_discard(&state.ui_c64);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        /* FIXME: this is ugly */
        c64_joystick_type_t joy_type = state.c64.joystick_type;
        state.c64.joystick_type = C64_JOYSTICKTYPE_NONE;
        c64_key_down(&state.c64, key_code);
        c64_key_up(&state.c64, key_code);
        state.c64.joystick_type = joy_type;
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 180;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else if (fs_ext("tap")) {
            load_success = c64_insert_tape(&state.c64, fs_ptr(), fs_size());
        }
        else if (fs_ext("bin") || fs_ext("prg") || fs_ext("")) {
            load_success = c64_quickload(&state.c64, fs_ptr(), fs_size());
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
            if (fs_ext("tap")) {
                c64_tape_play(&state.c64);
            }
            if (!sargs_exists("debug")) {
                if (sargs_exists("input")) {
                    keybuf_put(sargs_value("input"));
                }
                else if (fs_ext("tap")) {
                    keybuf_put("LOAD\n");
                }
                else if (fs_ext("prg")) {
                    keybuf_put("RUN\n");
                }
            }
        }
        else {
            gfx_flash_error();
        }
        fs_reset();
    }
}

static void draw_status_bar(void) {
    prof_push(PROF_FRAME, (float)state.frame_time_us * 0.001f);
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t frame_stats = prof_stats(PROF_FRAME);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", frame_stats.avg_val, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){
        .argc=argc,
        .argv=argv,
        .buf_size = 512 * 1024,
    });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * c64_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * c64_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "C64",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}

