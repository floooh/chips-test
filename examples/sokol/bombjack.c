/*
    bombjack.c

    Bomb Jack arcade machine emulation (MAME used as reference).
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80x.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "bombjack-roms.h"

// the actual emulator is here: https://github.com/floooh/chips/blob/master/systems/bombjack.h
#include "systems/bombjack.h"

static struct {
    bombjack_t sys;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    #ifdef CHIPS_USE_UI
        ui_bombjack_t ui;
    #endif
} state;

#ifdef CHIPS_USE_UI
#define BORDER_TOP (24)
#else
#define BORDER_TOP (8)
#endif
#define BORDER_LEFT (8)
#define BORDER_RIGHT (8)
#define BORDER_BOTTOM (32)

static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_bombjack_draw(&state.ui);
}
#endif

static void app_init(void) {
    gfx_init(&(gfx_desc_t) {
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border_left = BORDER_LEFT,
        .border_right = BORDER_RIGHT,
        .border_top = BORDER_TOP,
        .border_bottom = BORDER_BOTTOM,
        .emu_aspect_x = 4,
        .emu_aspect_y = 5,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    bombjack_init(&state.sys, &(bombjack_desc_t){
        .pixel_buffer = { .ptr = gfx_framebuffer(), .size = gfx_framebuffer_size() },
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .main_0000_1FFF = { .ptr=dump_09_j01b_bin, .size=sizeof(dump_09_j01b_bin) },
            .main_2000_3FFF = { .ptr=dump_10_l01b_bin, .size=sizeof(dump_10_l01b_bin) },
            .main_4000_5FFF = { .ptr=dump_11_m01b_bin, .size=sizeof(dump_11_m01b_bin) },
            .main_6000_7FFF = { .ptr=dump_12_n01b_bin, .size=sizeof(dump_12_n01b_bin) },
            .main_C000_DFFF = { .ptr=dump_13_1r, .size=sizeof(dump_13_1r) },
            .sound_0000_1FFF = { .ptr=dump_01_h03t_bin, .size=sizeof(dump_01_h03t_bin) },
            .chars_0000_0FFF = { .ptr=dump_03_e08t_bin, .size=sizeof(dump_03_e08t_bin) },
            .chars_1000_1FFF = { .ptr=dump_04_h08t_bin, .size=sizeof(dump_04_h08t_bin) },
            .chars_2000_2FFF = { .ptr=dump_05_k08t_bin, .size=sizeof(dump_05_k08t_bin) },
            .tiles_0000_1FFF = { .ptr=dump_06_l08t_bin, .size=sizeof(dump_06_l08t_bin) },
            .tiles_2000_3FFF = { .ptr=dump_07_n08t_bin, .size=sizeof(dump_07_n08t_bin) },
            .tiles_4000_5FFF = { .ptr=dump_08_r08t_bin, .size=sizeof(dump_08_r08t_bin) },
            .sprites_0000_1FFF = { .ptr=dump_16_m07b_bin, .size=sizeof(dump_16_m07b_bin) },
            .sprites_2000_3FFF = { .ptr=dump_15_l07b_bin, .size=sizeof(dump_15_l07b_bin) },
            .sprites_4000_5FFF = { .ptr=dump_14_j07b_bin, .size=sizeof(dump_14_j07b_bin) },
            .maps_0000_0FFF = { .ptr=dump_02_p04t_bin, .size=sizeof(dump_02_p04t_bin) }
        },
        #ifdef CHIPS_USE_UI
        .debug = ui_bombjack_get_debug(&state.ui),
        #endif
    });
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_bombjack_init(&state.ui, &(ui_bombjack_desc_t){
            .sys = &state.sys,
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
}

static void draw_status_bar(void);

static void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = bombjack_exec(&state.sys, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(bombjack_display_width(&state.sys), bombjack_display_height(&state.sys));
}

// input handling
static void app_input(const sapp_event* event) {
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        // input was handled by UI
        return;
    }
    #endif
    switch (event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                // player 1 joystick
                case SAPP_KEYCODE_RIGHT: state.sys.mainboard.p1 |= BOMBJACK_JOYSTICK_RIGHT; break;
                case SAPP_KEYCODE_LEFT:  state.sys.mainboard.p1 |= BOMBJACK_JOYSTICK_LEFT; break;
                case SAPP_KEYCODE_UP:    state.sys.mainboard.p1 |= BOMBJACK_JOYSTICK_UP; break;
                case SAPP_KEYCODE_DOWN:  state.sys.mainboard.p1 |= BOMBJACK_JOYSTICK_DOWN; break;
                case SAPP_KEYCODE_SPACE: state.sys.mainboard.p1 |= BOMBJACK_JOYSTICK_BUTTON; break;
                // player 1 coin
                case SAPP_KEYCODE_1:     state.sys.mainboard.sys |= BOMBJACK_SYS_P1_COIN; break;
                // player 1 start (any other key
                default:                 state.sys.mainboard.sys |= BOMBJACK_SYS_P1_START; break;
            }
            break;

        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                // player 1 joystick
                case SAPP_KEYCODE_RIGHT: state.sys.mainboard.p1 &= ~BOMBJACK_JOYSTICK_RIGHT; break;
                case SAPP_KEYCODE_LEFT:  state.sys.mainboard.p1 &= ~BOMBJACK_JOYSTICK_LEFT; break;
                case SAPP_KEYCODE_UP:    state.sys.mainboard.p1 &= ~BOMBJACK_JOYSTICK_UP; break;
                case SAPP_KEYCODE_DOWN:  state.sys.mainboard.p1 &= ~BOMBJACK_JOYSTICK_DOWN; break;
                case SAPP_KEYCODE_SPACE: state.sys.mainboard.p1 &= ~BOMBJACK_JOYSTICK_BUTTON; break;
                // player 1 coin
                case SAPP_KEYCODE_1:     state.sys.mainboard.sys &= ~BOMBJACK_SYS_P1_COIN; break;
                // player 1 start (any other key)
                default:                 state.sys.mainboard.sys &= ~BOMBJACK_SYS_P1_START; break;
            }
            break;
        default:
            break;
    }
}

static void app_cleanup(void) {
    bombjack_discard(&state.sys);
    #ifdef CHIPS_USE_UI
        ui_bombjack_discard(&state.ui);
    #endif
    saudio_shutdown();
    gfx_shutdown();
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
    (void)argc; (void)argv;
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = (3 * bombjack_std_display_width()) + BORDER_LEFT + BORDER_RIGHT,
        .height = (3 * bombjack_std_display_height()) + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Bomb Jack Arcade",
        .icon.sokol_default = true,
    };
}

