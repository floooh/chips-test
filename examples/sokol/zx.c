/*
    zx.c
    ZX Spectrum 48/128 emulator.
    - contended memory timing not emulated
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
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_ay38910.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_zx.h"
#endif

static struct {
    zx_t zx;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #if defined(CHIPS_USE_UI)
        ui_zx_t ui_zx;
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

// get zx_desc_t struct for given ZX type and joystick type
zx_desc_t zx_desc(zx_type_t type, zx_joystick_type_t joy_type) {
    return (zx_desc_t){
        .type = type,
        .joystick_type = joy_type,
        .pixel_buffer = { .ptr=gfx_framebuffer(), .size=gfx_framebuffer_size() },
        .audio = {
            .callback = { .func=push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .zx48k = { .ptr=dump_amstrad_zx48k_bin, .size=sizeof(dump_amstrad_zx48k_bin) },
            .zx128_0 = { .ptr=dump_amstrad_zx128k_0_bin, .size=sizeof(dump_amstrad_zx128k_0_bin) },
            .zx128_1 = { .ptr=dump_amstrad_zx128k_1_bin, .size=sizeof(dump_amstrad_zx128k_1_bin) },
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_zx_get_debug(&state.ui_zx)
        #endif
    };
}

#if defined(CHIPS_USE_UI)
void ui_draw_cb(void) {
    ui_zx_draw(&state.ui_zx);
}
static void ui_boot_cb(zx_t* sys, zx_type_t type) {
    zx_desc_t desc = zx_desc(type, sys->joystick_type);
    zx_init(sys, &desc);
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
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames=6 });
    clock_init();
    prof_init();
    saudio_setup(&(saudio_desc){0});
    fs_init();
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
    zx_init(&state.zx, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_zx_init(&state.ui_zx, &(ui_zx_desc_t){
            .zx = &state.zx,
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
    state.ticks = zx_exec(&state.zx, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(zx_display_width(&state.zx), zx_display_height(&state.zx));
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
    switch (event->type) {
        int c;
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                zx_key_down(&state.zx, c);
                zx_key_up(&state.zx, c);
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
                case SAPP_KEYCODE_LEFT_CONTROL: c = 0x0F; break; // SymShift
                default:                        c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    zx_key_down(&state.zx, c);
                }
                else {
                    zx_key_up(&state.zx, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    zx_discard(&state.zx);
    #ifdef CHIPS_USE_UI
        ui_zx_discard(&state.ui_zx);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        zx_key_down(&state.zx, key_code);
        zx_key_up(&state.zx, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 120;
    if (fs_ptr() && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext("txt") || fs_ext("bas")) {
            load_success = true;
            keybuf_put((const char*)fs_ptr());
        }
        else {
            load_success = zx_quickload(&state.zx, fs_ptr(), fs_size());
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
        fs_reset();
    }
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * zx_std_display_width() + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * zx_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "ZX Spectrum",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
    };
}

