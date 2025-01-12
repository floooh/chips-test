//------------------------------------------------------------------------------
//  z9001.c
//
//  Wiring diagram: http://www.sax.de/~zander/kc/kcsch_1.pdf
//  Detailed Manual: http://www.sax.de/~zander/z9001/doku/z9_fub.pdf
//
//  not emulated: beeper sound, border color, 40x20 video mode
//------------------------------------------------------------------------------
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/z80.h"
#include "chips/z80pio.h"
#include "chips/z80ctc.h"
#include "chips/beeper.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "chips/mem.h"
#include "systems/z9001.h"
#include "z9001-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_Z80
    #include "ui.h"
    #include "ui/ui_settings.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_display.h"
    #include "ui/ui_z80.h"
    #include "ui/ui_z80pio.h"
    #include "ui/ui_z80ctc.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_z9001.h"
#endif

#define SCREENSHOT_WIDTH (320)
#define SCREENSHOT_HEIGHT (192)

typedef struct {
    uint32_t version;
    z9001_t z9001;
} z9001_snapshot_t;

static struct {
    z9001_t z9001;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #ifdef CHIPS_USE_UI
        ui_z9001_t ui;
        z9001_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_boot_cb(z9001_t* sys, z9001_type_t type);
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

// audio-streaming callback
static void push_audio(const float* samples, int num_samples, void* user_data) {
    (void)user_data;
    saudio_push(samples, num_samples);
}

// get a z9001_desc_t struct for given Z9001 model
z9001_desc_t z9001_desc(z9001_type_t type) {
    return (z9001_desc_t) {
        .type = type,
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .z9001 = {
                .os_1  = { .ptr=dump_z9001_os12_1_bin, .size=sizeof(dump_z9001_os12_1_bin) },
                .os_2  = { .ptr=dump_z9001_os12_2_bin, .size=sizeof(dump_z9001_os12_2_bin) },
                .basic = { .ptr=dump_z9001_basic_507_511_bin, .size=sizeof(dump_z9001_basic_507_511_bin) },
                .font  = { .ptr=dump_z9001_font_bin, .size=sizeof(dump_z9001_font_bin) },
            },
            .kc87 = {
                .os    = { .ptr=dump_kc87_os_2_bin, .size=sizeof(dump_kc87_os_2_bin) },
                .basic = { .ptr=dump_z9001_basic_bin, .size=sizeof(dump_z9001_basic_bin) },
                .font  = { .ptr=dump_kc87_font_2_bin, .size=sizeof(dump_kc87_font_2_bin) }
            },
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_z9001_get_debug(&state.ui)
        #endif
    };
}

void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
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
        .display_info = z9001_display_info(0),
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames=12 });
    clock_init();
    prof_init();
    fs_init();
    z9001_type_t type = Z9001_TYPE_Z9001;
    if (sargs_exists("type")) {
        if (sargs_equals("type", "kc87")) {
            type = Z9001_TYPE_KC87;
        }
    }
    z9001_desc_t desc = z9001_desc(type);
    z9001_init(&state.z9001, &desc);
    #ifdef CHIPS_USE_UI
        ui_init(&(ui_desc_t){
            .draw_cb = ui_draw_cb,
            .save_settings_cb = ui_save_settings_cb,
            .imgui_ini_key = "floooh.chips.z9001",
        });
        ui_z9001_init(&state.ui, &(ui_z9001_desc_t){
            .z9001 = &state.z9001,
            .boot_cb = ui_boot_cb,
            .dbg_texture = {
                .create_cb = ui_create_texture,
                .update_cb = ui_update_texture,
                .destroy_cb = ui_destroy_texture,
            },
            .snapshot = {
                .load_cb = ui_load_snapshot,
                .save_cb = ui_save_snapshot,
                .empty_slot_screenshot = {
                    .texture = ui_shared_empty_snapshot_texture()
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
        ui_z9001_load_settings(&state.ui, ui_settings());
        ui_load_snapshots_from_storage();
    #endif
    bool delay_input = false;
    if (sargs_exists("file")) {
        delay_input = true;
        fs_load_file_async(FS_CHANNEL_IMAGES, sargs_value("file"));
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
    state.ticks = z9001_exec(&state.z9001, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(z9001_display_info(&state.z9001));
    handle_file_loading();
    send_keybuf_input();
}

// keyboard input handling
void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_load_dropped_file_async(FS_CHANNEL_IMAGES);
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
            if ((c >= 0x20) && (c < 0x7F)) {
                // need to invert case (unshifted is upper caps, shifted is lower caps
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                z9001_key_down(&state.z9001, c);
                z9001_key_up(&state.z9001, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_DOWN:
        case SAPP_EVENTTYPE_KEY_UP:
            switch (event->key_code) {
                case SAPP_KEYCODE_ENTER:    c = 0x0D; break;
                case SAPP_KEYCODE_RIGHT:    c = 0x09; break;
                case SAPP_KEYCODE_LEFT:     c = 0x08; break;
                case SAPP_KEYCODE_DOWN:     c = 0x0A; break;
                case SAPP_KEYCODE_UP:       c = 0x0B; break;
                case SAPP_KEYCODE_ESCAPE:   c = (event->modifiers & SAPP_MODIFIER_SHIFT)? 0x1B: 0x03; break;
                case SAPP_KEYCODE_INSERT:   c = 0x1A; break;
                case SAPP_KEYCODE_HOME:     c = 0x19; break;
                default:                    c = 0; break;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    z9001_key_down(&state.z9001, c);
                }
                else {
                    z9001_key_up(&state.z9001, c);
                }
            }
            break;
        default:
            break;
    }
}

// application cleanup callback
void app_cleanup(void) {
    z9001_discard(&state.z9001);
    #ifdef CHIPS_USE_UI
        ui_z9001_discard(&state.ui);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        z9001_key_down(&state.z9001, key_code);
        z9001_key_up(&state.z9001, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    if (fs_success(FS_CHANNEL_IMAGES) && clock_frame_count_60hz() > 20) {
        const chips_range_t file_data = fs_data(FS_CHANNEL_IMAGES);
        bool load_success = false;
        if (fs_ext(FS_CHANNEL_IMAGES, "txt") || (fs_ext(FS_CHANNEL_IMAGES, "bas"))) {
            load_success = true;
            keybuf_put((const char*)file_data.ptr);
        }
        else {
            load_success = z9001_quickload(&state.z9001, file_data);
        }
        if (load_success) {
            gfx_flash_success();
            if (sargs_exists("input")) {
                keybuf_put(sargs_value("input"));
            }
        }
        else {
            gfx_flash_error();
        }
        fs_reset(FS_CHANNEL_IMAGES);
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

#if defined(CHIPS_USE_UI)
static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    ui_z9001_draw(&state.ui, &(ui_z9001_frame_t){
        .display = draw_info->display,
    });
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_z9001_save_settings(&state.ui, settings);
}

static void ui_boot_cb(z9001_t* sys, z9001_type_t type) {
    z9001_desc_t desc = z9001_desc(type);
    z9001_init(sys, &desc);
}

static void ui_update_snapshot_screenshot(size_t slot) {
    ui_snapshot_screenshot_t screenshot = {
        .texture = ui_create_screenshot_texture(z9001_display_info(&state.snapshots[slot].z9001))
    };
    ui_snapshot_screenshot_t prev_screenshot = ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture) {
        ui_destroy_texture(prev_screenshot.texture);
    }
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = z9001_save_snapshot(&state.z9001, &state.snapshots[slot].z9001);
        ui_update_snapshot_screenshot(slot);
        fs_save_snapshot("z9001", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(z9001_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.snapshot.slots[slot].valid)) {
        success = z9001_load_snapshot(&state.z9001, state.snapshots[slot].version, &state.snapshots[slot].z9001);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(z9001_snapshot_t)) {
        return;
    }
    if (((z9001_snapshot_t*)response->data.ptr)->version != Z9001_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    ui_update_snapshot_screenshot(snapshot_slot);
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("z9001", snapshot_slot, ui_fetch_snapshot_callback);
    }
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    const chips_display_info_t info = z9001_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * info.screen.width + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * info.screen.height + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Robotron Z9001/KC87",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
        .html5_bubble_mouse_events = true,
        .html5_update_document_title = true,
        .logger.func = slog_func,
    };
}
