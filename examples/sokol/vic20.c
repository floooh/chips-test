/*
    vic20.c
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6522.h"
#include "chips/m6561.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "systems/vic20.h"
#include "vic20-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_m6522.h"
    #include "ui/ui_m6561.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_c1530.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_vic20.h"
#endif

static struct {
    vic20_t vic20;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    #ifdef CHIPS_USE_UI
        ui_vic20_t ui_vic20;
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

// get vic20_desc_t struct based on joystick type
vic20_desc_t vic20_desc(vic20_joystick_type_t joy_type, vic20_memory_config_t mem_config, bool c1530_enabled) {
    return (vic20_desc_t) {
        .c1530_enabled = c1530_enabled,
        .joystick_type = joy_type,
        .mem_config = mem_config,
        .audio = {
            .callback = { .func=push_audio },
            .sample_rate = saudio_sample_rate(),
            .volume = 0.3f,
        },
        .roms = {
            .chars = { .ptr=dump_vic20_characters_901460_03_bin, .size=sizeof(dump_vic20_characters_901460_03_bin) },
            .basic = { .ptr=dump_vic20_basic_901486_01_bin, .size=sizeof(dump_vic20_basic_901486_01_bin) },
            .kernal = { .ptr=dump_vic20_kernal_901486_07_bin, .size=sizeof(dump_vic20_kernal_901486_07_bin) },
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_vic20_get_debug(&state.ui_vic20)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_vic20_draw(&state.ui_vic20);
}
static void ui_boot_cb(vic20_t* sys) {
    vic20_desc_t desc = vic20_desc(sys->joystick_type, sys->mem_config, sys->c1530.valid);
    vic20_init(sys, &desc);
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
        .emu_aspect_x = 3,
        .emu_aspect_y = 2
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames=5 });
    clock_init();
    fs_init();
    saudio_setup(&(saudio_desc){0});
    vic20_joystick_type_t joy_type = VIC20_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        joy_type = VIC20_JOYSTICKTYPE_DIGITAL;
    }
    vic20_memory_config_t mem_config = VIC20_MEMCONFIG_STANDARD;
    if (sargs_exists("exp")) {
        if (sargs_equals("exp", "ram8k")) {
            mem_config = VIC20_MEMCONFIG_8K;
        }
        else if (sargs_equals("exp", "ram16k")) {
            mem_config = VIC20_MEMCONFIG_16K;
        }
        else if (sargs_equals("exp", "ram24k")) {
            mem_config = VIC20_MEMCONFIG_24K;
        }
        else if (sargs_equals("exp", "ram32k")) {
            mem_config = VIC20_MEMCONFIG_32K;
        }
        else if (sargs_equals("exp", "maxram")) {
            mem_config = VIC20_MEMCONFIG_MAX;
        }
    }
    bool c1530_enabled = sargs_exists("c1530");
    vic20_desc_t desc = vic20_desc(joy_type, mem_config, c1530_enabled);
    vic20_init(&state.vic20, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_vic20_init(&state.ui_vic20, &(ui_vic20_desc_t){
            .vic20 = &state.vic20,
            .boot_cb = ui_boot_cb,
            .create_texture_cb = gfx_create_texture,
            .update_texture_cb = gfx_update_texture,
            .destroy_texture_cb = gfx_destroy_texture,
            .dbg_keys = {
                .cont = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .stop = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F5), .name = "F5" },
                .step_over = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F6), .name = "F6" },
                .step_into = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F7), .name = "F7" },
                .step_tick = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F8), .name = "F8" },
                .toggle_breakpoint = { .keycode = simgui_map_keycode(SAPP_KEYCODE_F9), .name = "F9" }
            }
        });
    #endif
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        fs_start_load_file(sargs_value("file"));
    }
    if (sargs_exists("rom")) {
        fs_start_load_file(sargs_value("rom"));
    }
    if (sargs_exists("prg")) {
        if (!fs_load_base64("url.prg", sargs_value("prg"))) {
            gfx_flash_error();
        }
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

// per frame stuff, tick the emulator, handle input, decode and draw emulator display
void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = vic20_exec(&state.vic20, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(vic20_display_width(&state.vic20), vic20_display_height(&state.vic20));
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
                vic20_key_down(&state.vic20, c);
                vic20_key_up(&state.vic20, c);
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
                    vic20_key_down(&state.vic20, c);
                }
                else {
                    vic20_key_up(&state.vic20, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    vic20_discard(&state.vic20);
    #ifdef CHIPS_USE_UI
        ui_vic20_discard(&state.ui_vic20);
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
        vic20_joystick_type_t joy_type = state.vic20.joystick_type;
        state.vic20.joystick_type = VIC20_JOYSTICKTYPE_NONE;
        vic20_key_down(&state.vic20, key_code);
        vic20_key_up(&state.vic20, key_code);
        state.vic20.joystick_type = joy_type;
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
            load_success = vic20_insert_tape(&state.vic20, fs_ptr(), fs_size());
        }
        else if (fs_ext("bin") || fs_ext("prg") || fs_ext("")) {
            if (sargs_exists("rom")) {
                load_success = vic20_insert_rom_cartridge(&state.vic20, fs_ptr(), fs_size());
            }
            else {
                load_success = vic20_quickload(&state.vic20, fs_ptr(), fs_size());
            }
        }
        if (load_success) {
            if (clock_frame_count_60hz() > (load_delay_frames + 10)) {
                gfx_flash_success();
            }
            if (fs_ext("tap")) {
                vic20_tape_play(&state.vic20);
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
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    double frame_time_ms = state.frame_time_us / 1000.0f;
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms ticks:%d", frame_time_ms, state.exec_time_ms, state.ticks);
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
        .width = 3 * vic20_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * vic20_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "VIC-20",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}
