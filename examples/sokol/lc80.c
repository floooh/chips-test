/*
    lc80.c
*/
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_args.h"
#include "sokol_audio.h"
#include "sokol_debugtext.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "clock.h"
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/z80.h"
#include "chips/z80ctc.h"
#include "chips/z80pio.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "systems/lc80.h"
#include "lc80-roms.h"
#define UI_DBG_USE_Z80
#include "ui.h"
#include "ui/ui_settings.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_z80.h"
#include "ui/ui_z80pio.h"
#include "ui/ui_z80ctc.h"
#include "ui/ui_snapshot.h"
#include "ui/ui_lc80.h"

typedef struct {
    uint32_t version;
    lc80_t lc80;
} lc80_snapshot_t;

static struct {
    lc80_t lc80;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    ui_lc80_t ui;
    lc80_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
} state;

static void ui_boot_cb(lc80_t* sys);
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_save_snapshot(size_t slot_index);
static bool ui_load_snapshot(size_t slot_index);
static void ui_load_snapshots_from_storage(void);

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
        .debug = ui_lc80_get_debug(&state.ui),
    };
}

void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    sdtx_setup(&(sdtx_desc_t){
        .context_pool_size = 1,
        .fonts[0] = sdtx_font_oric(),
        .logger.func = slog_func,
    });
    clock_init();
    prof_init();
    fs_init();

    lc80_desc_t desc = lc80_desc();
    lc80_init(&state.lc80, &desc);

    ui_init(&(ui_desc_t){
        .draw_cb = ui_draw_cb,
        .save_settings_cb = ui_save_settings_cb,
        .imgui_ini_key = "floooh.chips.lc80",
    });
    ui_lc80_init(&state.ui, &(ui_lc80_desc_t){
        .sys = &state.lc80,
        .boot_cb = ui_boot_cb,
        .dbg_texture = {
            .create_cb = ui_create_texture,
            .update_cb = ui_update_texture,
            .destroy_cb = ui_destroy_texture,
        },
        .snapshot = {
            .load_cb = ui_load_snapshot,
            .save_cb = ui_save_snapshot,
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
    ui_lc80_load_settings(&state.ui, ui_settings());
    ui_load_snapshots_from_storage();
}

static void draw_status_bar(void);

void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = lc80_exec(&state.lc80, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    sg_begin_pass(&(sg_pass){ .swapchain = sglue_swapchain() });
    ui_draw(0);
    sdtx_draw();
    sg_end_pass();
    sg_commit();
    fs_dowork();
}

void app_input(const sapp_event* event) {
    ui_input(event);
}

void app_cleanup(void) {
    lc80_discard(&state.lc80);
    ui_lc80_discard(&state.ui);
    saudio_shutdown();
    sdtx_shutdown();
    sg_shutdown();
    sargs_shutdown();
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

static void ui_boot_cb(lc80_t* sys) {
    lc80_desc_t desc = lc80_desc();
    lc80_init(sys, &desc);
}

static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    (void)draw_info;
    ui_lc80_draw(&state.ui);
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_lc80_save_settings(&state.ui, settings);
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = lc80_save_snapshot(&state.lc80, &state.snapshots[slot].lc80);
        state.ui.win.snapshot.slots[slot].valid = true;
        fs_save_snapshot("lc80", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(lc80_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.win.snapshot.slots[slot].valid)) {
        success = lc80_load_snapshot(&state.lc80, state.snapshots[slot].version, &state.snapshots[slot].lc80);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(lc80_snapshot_t)) {
        return;
    }
    if (((lc80_snapshot_t*)response->data.ptr)->version != LC80_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    state.ui.win.snapshot.slots[snapshot_slot].valid = true;
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("lc80", snapshot_slot, ui_fetch_snapshot_callback);
    }
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
        .html5_bubble_mouse_events = true,
        .html5_update_document_title = true,
        .logger.func = slog_func,
    };
}
