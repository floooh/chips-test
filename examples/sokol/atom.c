/*
    atom.c

    The Acorn Atom was a very simple 6502-based home computer
    (just a MOS 6502 CPU, Motorola MC6847 video
    display generator, and Intel i8255 I/O chip).

    Note: Ctrl+L (clear screen) is mapped to F1.

    NOT EMULATED:
        - REPT key (and some other special keys)
*/
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "common.h"
#include "chips/m6502.h"
#include "chips/mc6847.h"
#include "chips/i8255.h"
#include "chips/m6522.h"
#include "chips/beeper.h"
#include "chips/clk.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "systems/atom.h"
#include "atom-roms.h"
#if defined(CHIPS_USE_UI)
    #define UI_DBG_USE_M6502
    #include "ui.h"
    #include "ui/ui_settings.h"
    #include "ui/ui_chip.h"
    #include "ui/ui_memedit.h"
    #include "ui/ui_memmap.h"
    #include "ui/ui_dasm.h"
    #include "ui/ui_dbg.h"
    #include "ui/ui_m6502.h"
    #include "ui/ui_m6522.h"
    #include "ui/ui_mc6847.h"
    #include "ui/ui_i8255.h"
    #include "ui/ui_audio.h"
    #include "ui/ui_display.h"
    #include "ui/ui_kbd.h"
    #include "ui/ui_snapshot.h"
    #include "ui/ui_atom.h"
#endif

typedef struct {
    uint32_t version;
    atom_t atom;
} atom_snapshot_t;

static struct {
    atom_t atom;
    uint32_t frame_time_us;
    uint32_t ticks;
    double emu_time_ms;
    #ifdef CHIPS_USE_UI
        ui_atom_t ui;
        atom_snapshot_t snapshots[UI_SNAPSHOT_MAX_SLOTS];
    #endif
} state;

#ifdef CHIPS_USE_UI
static void ui_draw_cb(const ui_draw_info_t* draw_info);
static void ui_save_settings_cb(ui_settings_t* settings);
static void ui_boot_cb(atom_t* sys);
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

atom_desc_t atom_desc(atom_joystick_type_t joy_type) {
    return (atom_desc_t) {
        .joystick_type = joy_type,
        .audio = {
            .callback = { .func = push_audio },
            .sample_rate = saudio_sample_rate(),
        },
        .roms = {
            .abasic = { .ptr=dump_abasic_ic20, .size = sizeof(dump_abasic_ic20) },
            .afloat = { .ptr=dump_afloat_ic21, .size = sizeof(dump_afloat_ic21) },
            .dosrom = { .ptr=dump_dosrom_u15, .size = sizeof(dump_dosrom_u15) }
        },
        #if defined(CHIPS_USE_UI)
        .debug = ui_atom_get_debug(&state.ui)
        #endif
    };
}

void app_init(void) {
    saudio_setup(&(saudio_desc){
        .logger.func = slog_func,
    });
    atom_joystick_type_t joy_type = ATOM_JOYSTICKTYPE_NONE;
    if (sargs_exists("joystick")) {
        if (sargs_equals("joystick", "mmc") || sargs_equals("joystick", "yes")) {
            joy_type = ATOM_JOYSTICKTYPE_MMC;
        }
    }
    atom_desc_t desc = atom_desc(joy_type);
    atom_init(&state.atom, &desc);
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
        .display_info = atom_display_info(&state.atom)
    });
    keybuf_init(&(keybuf_desc_t){ .key_delay_frames = 10 });
    clock_init();
    prof_init();
    fs_init();
    #ifdef CHIPS_USE_UI
        ui_init(&(ui_desc_t){
            .draw_cb = ui_draw_cb,
            .save_settings_cb = ui_save_settings_cb,
            .imgui_ini_key = "floooh.chips.atom",
        });
        ui_atom_init(&state.ui, &(ui_atom_desc_t){
            .atom = &state.atom,
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
        ui_atom_load_settings(&state.ui, ui_settings());
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
    state.ticks = atom_exec(&state.atom, state.frame_time_us);
    state.emu_time_ms = stm_ms(stm_since(emu_start_time));
    draw_status_bar();
    gfx_draw(atom_display_info(&state.atom));
    handle_file_loading();
    send_keybuf_input();
}

/* keyboard input handling */
void app_input(const sapp_event* event) {
    // accept dropped files also when ImGui grabs input
    if (event->type == SAPP_EVENTTYPE_FILES_DROPPED) {
        fs_load_dropped_file_async(FS_CHANNEL_IMAGES);
    }
    #ifdef CHIPS_USE_UI
    if (ui_input(event)) {
        /* input was handled by UI */
        return;
    }
    #endif
    int c = 0;
    switch (event->type) {
        case SAPP_EVENTTYPE_CHAR:
            c = (int) event->char_code;
            if ((c > 0x20) && (c < 0x7F)) {
                /* need to invert case (unshifted is upper caps, shifted is lower caps */
                if (isupper(c)) {
                    c = tolower(c);
                }
                else if (islower(c)) {
                    c = toupper(c);
                }
                atom_key_down(&state.atom, c);
                atom_key_up(&state.atom, c);
            }
            break;
        case SAPP_EVENTTYPE_KEY_UP:
        case SAPP_EVENTTYPE_KEY_DOWN:
            switch (event->key_code) {
                case SAPP_KEYCODE_SPACE:        c = 0x20; break;
                case SAPP_KEYCODE_RIGHT:        c = 0x09; break;
                case SAPP_KEYCODE_LEFT:         c = 0x08; break;
                case SAPP_KEYCODE_DOWN:         c = 0x0A; break;
                case SAPP_KEYCODE_UP:           c = 0x0B; break;
                case SAPP_KEYCODE_ENTER:        c = 0x0D; break;
                case SAPP_KEYCODE_INSERT:       c = 0x1A; break;
                case SAPP_KEYCODE_HOME:         c = 0x19; break;
                case SAPP_KEYCODE_BACKSPACE:    c = 0x01; break;
                case SAPP_KEYCODE_ESCAPE:       c = 0x1B; break;
                case SAPP_KEYCODE_F1:           c = 0x0C; break; /* mapped to Ctrl+L (clear screen) */
                default:                        c = 0;
            }
            if (c) {
                if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
                    atom_key_down(&state.atom, c);
                }
                else {
                    atom_key_up(&state.atom, c);
                }
            }
            break;
        default:
            break;
    }
}

void app_cleanup(void) {
    atom_discard(&state.atom);
    #ifdef CHIPS_USE_UI
        ui_atom_discard(&state.ui);
        ui_discard();
    #endif
    saudio_shutdown();
    gfx_shutdown();
    sargs_shutdown();
}

static void send_keybuf_input(void) {
    uint8_t key_code;
    if (0 != (key_code = keybuf_get(state.frame_time_us))) {
        atom_key_down(&state.atom, key_code);
        atom_key_up(&state.atom, key_code);
    }
}

static void handle_file_loading(void) {
    fs_dowork();
    const uint32_t load_delay_frames = 48;
    if (fs_success(FS_CHANNEL_IMAGES) && clock_frame_count_60hz() > load_delay_frames) {
        bool load_success = false;
        if (fs_ext(FS_CHANNEL_IMAGES, "txt") || fs_ext(FS_CHANNEL_IMAGES, "bas")) {
            load_success = true;
            keybuf_put((const char*)fs_data(FS_CHANNEL_IMAGES).ptr);
        }
        if (fs_ext(FS_CHANNEL_IMAGES, "tap")) {
            load_success = atom_insert_tape(&state.atom, fs_data(FS_CHANNEL_IMAGES));
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
    ui_atom_draw(&state.ui, &(ui_atom_frame_t){
        .display = draw_info->display,
    });
}

static void ui_save_settings_cb(ui_settings_t* settings) {
    ui_atom_save_settings(&state.ui, settings);
}

static void ui_boot_cb(atom_t* sys) {
    atom_desc_t desc = atom_desc(sys->joystick_type);
    atom_init(sys, &desc);
}

static void ui_update_snapshot_screenshot(size_t slot) {
    ui_snapshot_screenshot_t screenshot = {
        .texture = ui_create_screenshot_texture(atom_display_info(&state.snapshots[slot].atom))
    };
    ui_snapshot_screenshot_t prev_screenshot = ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture) {
        ui_destroy_texture(prev_screenshot.texture);
    }
}

static void ui_save_snapshot(size_t slot) {
    if (slot < UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = atom_save_snapshot(&state.atom, &state.snapshots[slot].atom);
        ui_update_snapshot_screenshot(slot);
        fs_save_snapshot("atom", slot, (chips_range_t){ .ptr = &state.snapshots[slot], sizeof(atom_snapshot_t) });
    }
}

static bool ui_load_snapshot(size_t slot) {
    bool success = false;
    if ((slot < UI_SNAPSHOT_MAX_SLOTS) && (state.ui.snapshot.slots[slot].valid)) {
        success = atom_load_snapshot(&state.atom, state.snapshots[slot].version, &state.snapshots[slot].atom);
    }
    return success;
}

static void ui_fetch_snapshot_callback(const fs_snapshot_response_t* response) {
    assert(response);
    if (response->result != FS_RESULT_SUCCESS) {
        return;
    }
    if (response->data.size != sizeof(atom_snapshot_t)) {
        return;
    }
    if (((atom_snapshot_t*)response->data.ptr)->version != ATOM_SNAPSHOT_VERSION) {
        return;
    }
    size_t snapshot_slot = response->snapshot_index;
    assert(snapshot_slot < UI_SNAPSHOT_MAX_SLOTS);
    memcpy(&state.snapshots[snapshot_slot], response->data.ptr, response->data.size);
    ui_update_snapshot_screenshot(snapshot_slot);
}

static void ui_load_snapshots_from_storage(void) {
    for (size_t snapshot_slot = 0; snapshot_slot < UI_SNAPSHOT_MAX_SLOTS; snapshot_slot++) {
        fs_load_snapshot_async("atom", snapshot_slot, ui_fetch_snapshot_callback);
    }
}
#endif

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc=argc, .argv=argv });
    const chips_display_info_t info = atom_display_info(0);
    return (sapp_desc) {
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 2 * info.screen.width + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * info.screen.height + BORDER_TOP + BORDER_BOTTOM,
        .window_title = "Acorn Atom",
        .icon.sokol_default = true,
        .enable_dragndrop = true,
        .html5_bubble_mouse_events = true,
        .html5_update_document_title = true,
        .logger.func = slog_func,
    };
}
