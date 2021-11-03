/*
    lc80.c
*/
#include "common.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_args.h"
#include "sokol_audio.h"
#include "sokol_debugtext.h"
#include "sokol_glue.h"
#include "clock.h"
#define CHIPS_IMPL
#include "chips/z80x.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "systems/lc80.h"
#include "lc80-roms.h"
#define UI_DBG_USE_Z80
#include "ui.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_lc80.h"

static struct {
    lc80_t lc80;
    uint32_t frame_time_us;
    uint32_t ticks;
    double exec_time_ms;
    ui_lc80_t ui_lc80;
} state;

static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

static lc80_desc_t lc80_desc(void) {
    return (lc80_desc_t) {
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .rom = { .ptr = dump_lc80_2k_bin, .size = sizeof(dump_lc80_2k_bin) },
        .debug = ui_lc80_get_debug(&state.ui_lc80),
    };
}

static void ui_boot_cb(lc80_t* sys) {
    lc80_desc_t desc = lc80_desc();
    lc80_init(sys, &desc);
}

static void ui_draw_cb(void) {
    ui_lc80_draw(&state.ui_lc80);
}

void app_init(void) {
    sg_setup(&(sg_desc){ .context = sapp_sgcontext() });
    sdtx_setup(&(sdtx_desc_t){
        .context_pool_size = 1,
        .fonts[0] = sdtx_font_oric()
    });
    clock_init();
    saudio_setup(&(saudio_desc){0});

    lc80_desc_t desc = lc80_desc();
    lc80_init(&state.lc80, &desc);

    ui_init(ui_draw_cb);
    ui_lc80_init(&state.ui_lc80, &(ui_lc80_desc_t){
        .sys = &state.lc80,
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
}

static void draw_status_bar(void);

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t exec_start_time = stm_now();
    state.ticks = lc80_exec(&state.lc80, state.frame_time_us);
    state.exec_time_ms = stm_ms(stm_since(exec_start_time));
    draw_status_bar();
    sg_begin_default_pass(&(sg_pass_action){0}, sapp_width(), sapp_height());
    ui_draw();
    sdtx_draw();
    sg_end_pass();
    sg_commit();
}

void app_input(const sapp_event* event) {
    ui_input(event);
}

void app_cleanup(void) {
    lc80_discard(&state.lc80);
    ui_lc80_discard(&state.ui_lc80);
    saudio_shutdown();
    sdtx_shutdown();
    sg_shutdown();
    sargs_shutdown();
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
    sargs_setup(&(sargs_desc) { .argc = argc, .argv = argv });
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 1024,
        .height = 720,
        .window_title = "LC-80",
        .icon.sokol_default = true,
    };
}
