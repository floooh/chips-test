/*
    pengo.c

    Pengo arcade machine emulator.
*/
#include "common.h"
#define CHIPS_IMPL
#include "chips/z80x.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "pengo-roms.h"
#define NAMCO_PENGO
#include "systems/namco.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_namco.h"
#endif

static struct {
    namco_t sys;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    #ifdef CHIPS_USE_UI
        ui_namco_t ui;
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

static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_namco_draw(&state.ui);
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
        .emu_aspect_x = 2,
        .emu_aspect_y = 3,
        .rot90 = true
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});
    namco_init(&state.sys, &(namco_desc_t){
        .pixel_buffer = { .ptr = gfx_framebuffer(), .size = gfx_framebuffer_size() },
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .common = {
                .cpu_0000_0FFF = { .ptr=dump_ep5120_8, .size=sizeof(dump_ep5120_8) },
                .cpu_1000_1FFF = { .ptr=dump_ep5121_7, .size=sizeof(dump_ep5121_7) },
                .cpu_2000_2FFF = { .ptr=dump_ep5122_15, .size=sizeof(dump_ep5122_15) },
                .cpu_3000_3FFF = { .ptr=dump_ep5123_14, .size=sizeof(dump_ep5123_14) },
                .prom_0000_001F = { .ptr=dump_pr1633_78, .size=sizeof(dump_pr1633_78) },
                .sound_0000_00FF = { .ptr=dump_pr1635_51, .size=sizeof(dump_pr1635_51) },
                .sound_0100_01FF = { .ptr=dump_pr1636_70, .size=sizeof(dump_pr1636_70) }
            },
            .pengo = {
                .cpu_4000_4FFF = { .ptr=dump_ep5124_21, .size=sizeof(dump_ep5124_21) },
                .cpu_5000_5FFF = { .ptr=dump_ep5125_20, .size=sizeof(dump_ep5125_20) },
                .cpu_6000_6FFF = { .ptr=dump_ep5126_32, .size=sizeof(dump_ep5126_32) },
                .cpu_7000_7FFF = { .ptr=dump_ep5127_31, .size=sizeof(dump_ep5127_31) },
                .gfx_0000_1FFF = { .ptr=dump_ep1640_92, .size=sizeof(dump_ep1640_92) },
                .gfx_2000_3FFF = { .ptr=dump_ep1695_105, .size=sizeof(dump_ep1695_105) },
                .prom_0020_041F = { .ptr=dump_pr1634_88, .size=sizeof(dump_pr1634_88) }
            }
        },
        #ifdef CHIPS_USE_UI
        .debug = ui_namco_get_debug(&state.ui),
        #endif
    });
    #ifdef CHIPS_USE_UI
        ui_init(ui_draw_cb);
        ui_namco_init(&state.ui, &(ui_namco_desc_t){
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
    state.ticks = namco_exec(&state.sys, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    gfx_draw(namco_display_width(&state.sys), namco_display_height(&state.sys));
}

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
                case SAPP_KEYCODE_RIGHT:    namco_input_set(&state.sys, NAMCO_INPUT_P1_RIGHT); break;
                case SAPP_KEYCODE_LEFT:     namco_input_set(&state.sys, NAMCO_INPUT_P1_LEFT); break;
                case SAPP_KEYCODE_UP:       namco_input_set(&state.sys, NAMCO_INPUT_P1_UP); break;
                case SAPP_KEYCODE_DOWN:     namco_input_set(&state.sys, NAMCO_INPUT_P1_DOWN); break;
                case SAPP_KEYCODE_1:        namco_input_set(&state.sys, NAMCO_INPUT_P1_COIN); break;
                case SAPP_KEYCODE_2:        namco_input_set(&state.sys, NAMCO_INPUT_P2_COIN); break;
                case SAPP_KEYCODE_SPACE:    namco_input_set(&state.sys, NAMCO_INPUT_P1_BUTTON); break;
                default:                    namco_input_set(&state.sys, NAMCO_INPUT_P1_START); break;
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_RIGHT:    namco_input_clear(&state.sys, NAMCO_INPUT_P1_RIGHT); break;
                case SAPP_KEYCODE_LEFT:     namco_input_clear(&state.sys, NAMCO_INPUT_P1_LEFT); break;
                case SAPP_KEYCODE_UP:       namco_input_clear(&state.sys, NAMCO_INPUT_P1_UP); break;
                case SAPP_KEYCODE_DOWN:     namco_input_clear(&state.sys, NAMCO_INPUT_P1_DOWN); break;
                case SAPP_KEYCODE_1:        namco_input_clear(&state.sys, NAMCO_INPUT_P1_COIN); break;
                case SAPP_KEYCODE_2:        namco_input_clear(&state.sys, NAMCO_INPUT_P2_COIN); break;
                case SAPP_KEYCODE_SPACE:    namco_input_clear(&state.sys, NAMCO_INPUT_P1_BUTTON); break;
                default:                    namco_input_clear(&state.sys, NAMCO_INPUT_P1_START); break;
            }
            break;
        default:
            break;
    }
}

static void app_cleanup(void) {
    namco_discard(&state.sys);
    #ifdef CHIPS_USE_UI
        ui_namco_discard(&state.ui);
        ui_discard();
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
        .width = ((5 * namco_std_display_width()) / 2) + BORDER_LEFT + BORDER_RIGHT,
        .height = 3 * namco_std_display_height() + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Pengo Arcade",
        .icon.sokol_default = true,
    };
}

