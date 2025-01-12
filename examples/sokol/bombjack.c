/*
    bombjack.c

    Bomb Jack arcade machine emulation (MAME used as reference).
*/
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/z80.h"
#include "chips/ay38910.h"
#include "chips/clk.h"
#include "chips/mem.h"
// the actual emulator is here: https://github.com/floooh/chips/blob/master/systems/bombjack.h
#include "systems/bombjack.h"
#include "bombjack-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_settings.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_ay38910.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_display.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_bombjack.h"
#endif

typedef struct {
    uint32_t version;
    bombjack_t sys;
} bombjack_snapshot_t;

static struct {
    bombjack_t sys;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #ifdef CHIPS_USE_UI
        ui_bombjack_t ui;
        bombjack_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_save_snapshot(size_t slot_index);
static bool ui_load_snapshot(size_t slot_index);
static void ui_load_snapshots_from_storage(void);
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

static void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    bombjack_init(&state.sys, &(bombjack_desc_t){
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
    gfx_init(&(gfx_desc_t) {
        .disable_speaker_icon = sargs_exists("disable-speaker-icon"),
        #ifdef CHIPS_USE_UI
        .draw_extra_cb = ui_draw,
        #endif
        .border = {
            .left = BORDER_LEFT,
            .right = BORDER_RIGHT,
            .top = BORDER_TOP,
            .bottom = BORDER_BOTTOM,
        },
        .display_info = bombjack_display_info(&state.sys),
        .pixel_aspect = {
            .width = 4,
            .height = 5,
        }
    });
    clock_init();
    prof_init();
    fs_init();
    #ifdef CHIPS_USE_UI
        ui_init(&(ui_desc_t){
            .draw_cb = ui_draw_cb,
            .save_settings_cb = ui_save_settings_cb,
            .imgui_ini_key = "floooh.chips.bombjack",
        });
        ui_bombjack_init(&state.ui, &(ui_bombjack_desc_t){
            .sys = &state.sys,
            .dbg_texture = {
                .create_cb = ui_create_texture,
                .update_cb = ui_update_texture,
                .destroy_cb = ui_destroy_texture,
            },
            .snapshot = {
                .load_cb = ui_load_snapshot,
                .save_cb = ui_save_snapshot,
                .empty_slot_screenshot = {
                    .texture = ui_shared_empty_snapshot_texture(),
                    .portrait = true,
                }
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
        ui_bombjack_load_settings(&state.ui, ui_settings());
        ui_load_snapshots_from_storage();
    #endif
}

static void draw_status_bar(void);

static void app_frame(void) {
    state.frame_time_us = clock_frame_time();
    const uint64_t emu_start_time = stm_now();
    state.ticks = bombjack_exec(&state.sys, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(bombjack_display_info(&state.sys));
    fs_dowork();
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
    prof_push(PROF_EMU, (float)state.emu_time_ms);
    prof_stats_t emu_stats = prof_stats(PROF_EMU);
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    sdtx_canvas(w, h);
    sdtx_color3b(255, 255, 255);
    sdtx_pos(1.0f, (h / 8.0f) - 1.5f);
    sdtx_printf("frame:%.2fms emu:%.2fms (min:%.2fms max:%.2fms) ticks:%d", (float)state.frame_time_us * 0.001f, emu_stats.avg_val, emu_stats.min_val, emu_stats.max_val, state.ticks);
}

#if defined(CHIPS_USE_UI)

static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    ui_bombjack_draw(&state.ui, &(ui_bombjack_frame_t){
        .display = draw_info->display,
    });
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_bombjack_save_settings(&state.ui, settings);
}

static void ui_update_snapshot_screenshot(size_t slot) {
    const ui_snapshot_screenshot_t screenshot = {
        .texture = ui_create_screenshot_texture(bombjack_display_info(&state.snapshots[slot].sys)),
        .portrait = true,
    };
    ui_snapshot_screenshot_t prev_screenshot = ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture) {
        ui_destroy_texture(prev_screenshot.texture);
    }
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = bombjack_save_snapshot(&state.sys, &state.snapshots[slot].sys);
        ui_update_snapshot_screenshot(slot);
        fs_save_snapshot("bombjack", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(bombjack_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.snapshot.slots[slot].valid)) {
        success = bombjack_load_snapshot(&state.sys, state.snapshots[slot].version, &state.snapshots[slot].sys);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(bombjack_snapshot_t)) {
        return;
    }
    if (((bombjack_snapshot_t*)response->data.ptr)->version != BOMBJACK_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    ui_update_snapshot_screenshot(snapshot_slot);
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("bombjack", snapshot_slot, ui_fetch_snapshot_callback);
    }
}

#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    const chips_display_info_t info = bombjack_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = (3 * info.screen.width) + BORDER_LEFT + BORDER_RIGHT,
        .height = (3 * info.screen.height) + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Bomb Jack Arcade",
        .icon.sokol_default = true,
        .html5_bubble_mouse_events = true,
        .html5_update_document_title = true,
        .logger.func = slog_func,
    };
}
