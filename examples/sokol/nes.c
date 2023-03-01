/*
    nes.c

    NES.
*/
#include <stdio.h>
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "chips/m6502.h"
#include "chips/r2c02.h"
#include "systems/nes.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_nes.h"
#endif

static struct {
    nes_t nes;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #if defined(CHIPS_USE_UI)
        ui_nes_t ui;
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(void);
#endif

static void draw_status_bar(void);

static void app_init(void) {
    nes_init(&state.nes, &(nes_desc_t) {
        #if defined(CHIPS_USE_UI)
        .debug = ui_nes_get_debug(&state.ui)
        #endif
    });
    gfx_init(&(gfx_desc_t){
    #ifdef CHIPS_USE_UI
    .draw_extra_cb = ui_draw,
    #endif
    .display_info = nes_display_info(&state.nes),
    });
    clock_init();
    prof_init();
    fs_init();

#ifdef CHIPS_USE_UI
    ui_init(ui_draw_cb);
    ui_nes_init(&state.ui, &(ui_nes_desc_t){
        .nes = &state.nes,
        .dbg_texture = {
            .create_cb = gfx_create_texture,
            .update_cb = gfx_update_texture,
            .destroy_cb = gfx_destroy_texture,
        },
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

    if (sargs_exists("file")) {
        fs_start_load_file(FS_SLOT_IMAGE, sargs_value("file"));
    }
}

static void handle_file_loading(void);

static void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = nes_exec(&state.nes, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(nes_display_info(&state.nes));
    handle_file_loading();
}

static void app_cleanup(void) {
    nes_discard(&state.nes);
    #ifdef CHIPS_USE_UI
        ui_nes_discard(&state.ui);
        ui_discard();
    #endif
    gfx_shutdown();
    sargs_shutdown();
}

static void draw_status_bar(void) {
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);

    const uint32_t text_color = 0xFFFFFFFF;

    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_origin(1.0f, (h / 8.0f) - 3.5f);

    sdtx_font(0);
    sdtx_color1i(text_color);
    sdtx_pos(0.0f, 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

static void handle_file_loading(void) {
    fs_dowork();
    if (fs_success(FS_SLOT_IMAGE)) {
        bool load_success = false;
        if (fs_ext(FS_SLOT_IMAGE, "nes")) {
            load_success = nes_insert_cart(&state.nes, fs_data(FS_SLOT_IMAGE));
        }
        fs_reset(FS_SLOT_IMAGE);
    }
}

void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_start_load_dropped_file(FS_SLOT_IMAGE);
    }
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
       // input was handled by UI
       return;
    }
    #endif
}

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(void) {
    ui_nes_draw(&state.ui);
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    //const chips_display_info_t info = nes_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .event_cb = app_input,
        .frame_cb = app_frame,
        .cleanup_cb = app_cleanup,
        // .width = info.screen.width,
        // .height = info.screen.height,
        .width = 800,
        .height = 600,
        .window_title = "NES",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}